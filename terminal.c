

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/util/queue.h"
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#include "terminal.h"
#include "terminal_commands.h"


#define UART_ID uart0
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE


#define UART_TX_PIN 0
#define UART_RX_PIN 1

#define MAX_ARGS 16

#define TX_BUF_SIZE (256)
#define RX_BUF_SIZE (256)

queue_t term_tx;
uint8_t term_tx_buf[TX_BUF_SIZE];

#define CMD_BUF_SIZE 256
#define CMD_SIZE 256

queue_t cmd_buf;
static uint8_t cmd_buffer[CMD_BUF_SIZE];

static uint8_t command[CMD_SIZE];
static uint8_t current_command[CMD_SIZE];
static uint8_t cmd_len;

static bool process_commands;

static char *argv[MAX_ARGS];

static const term_command_t *commands;
static uint16_t command_count;

static uint8_t term_data[TX_BUF_SIZE];

// Uart interrupt handler
void on_uart_interrupt()
{
	uint8_t ch;

    while (uart_is_readable(UART_ID))
    {
        ch = uart_getc(UART_ID);
        if (uart_is_writable(UART_ID))
        {
            uart_putc(UART_ID, ch);
            if(ch == 0x0D)
            uart_putc(UART_ID, 0x0A);
        }
        queue_try_add(&cmd_buf, &ch);
    }
    while (uart_is_writable(UART_ID) && !queue_is_empty(&term_tx))
    {
		if(queue_try_remove(&term_tx, &ch))
		{
			uart_putc(UART_ID, ch);
		}
    }
	//clear all interrupts
	uart_get_hw(UART_ID)->icr = uart_get_hw(UART_ID)->mis;
}

void term_init(void)
{
    // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 2400);

    // Set the TX and RX pins by using the function select on the GPIO
    // Set datasheet for more information on function select
    gpio_set_function(UART_TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(UART_RX_PIN, GPIO_FUNC_UART);

    // Actually, we want a different speed
    // The call will return the actual baud rate selected, which will be as close as
    // possible to that requested
    int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);

    // Set UART flow control CTS/RTS, we don't want these, so turn them off
    uart_set_hw_flow(UART_ID, false, false);

    // Set our data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_interrupt);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts - RX only
    uart_set_irq_enables(UART_ID, true, true);

	cmd_len = 0;
	queue_init(&cmd_buf, sizeof(cmd_buffer[0]), CMD_BUF_SIZE);

	queue_init(&term_tx, sizeof(term_tx_buf[0]), TX_BUF_SIZE);

	commands = get_commands();
	command_count = get_command_count();
}

void uart_transmit(uart_inst_t* uart, char* dat)
{
	uint8_t ch;

	while (*dat)
	{
		queue_try_add(&term_tx, dat);
		dat++;
	}
	//trigger uart interrupt here. 
    if(uart_is_writable(UART_ID))
    {
		if(queue_try_remove(&term_tx, &ch))
		{
			uart_putc(UART_ID, ch);
		}
    }
}

void print_va (const char *format, va_list ap)
{
    vsprintf((char*)term_data, format, ap);

    uart_transmit(UART_ID, term_data);
}


void print(const char *format, ...)
{
    va_list ap;
    uint32_t res;

    va_start (ap, format);
    print_va (format, ap);
    va_end (ap);
}

static void run_command(int argc)
{
	uint16_t i;
	int ret_val;
	
	for (i = 0; i < command_count; i++)
	{
		if(!commands[i].command)
			break;
		
		if (strcmp(argv[0], commands[i].command) == 0)
		{
			ret_val = commands[i].handler(argc, argv);
			if(ret_val == 1)
			{
				char* help_argv[2] = {"help"};
				help_argv[1] = argv[0];
				commands[0].handler (2, help_argv);
			}
			return;
		}
	}
	print("Invalid Command: %s\r\n", argv[0]);
}

static void parse_command(uint8_t* cmd, uint8_t* cmd_len)
{
	uint8_t argc;
	char *pos;
	bool space = true;
	
	strncpy(current_command, cmd, CMD_SIZE);

	command[0] = 0x00;
	*cmd_len = 0x00;

	pos = current_command;

	for (argc = 0; *pos && argc < MAX_ARGS; pos++)
	{
		if(space)		
		{
			if (*pos != ' ')
			{
				argv[argc++] = pos;
				space = false;
			}
		}
		else if (*pos == ' ')
		{
			*pos = 0;
			space = true;
		}
	}
	
	if (argc >= 1)
	{
		run_command(argc);
	}
			
}

void process_command(uint8_t* cmd, uint8_t* cmd_len)
{
	parse_command(cmd, cmd_len);
    //uart_puts(UART_ID, cmd);
    //uart_puts(UART_ID, "\r\n");
	//*cmd_len = 0x00;
}

void terminal_task(void)
{
	//uint32_t dw_status = usart_get_status(CONSOLE_UART);
	uint8_t c;

    //if there are bytes to transmit, and bytes are waiting to be transmitted
    //then enable the interrupt 
    /*
	if (dw_status & US_CSR_TXRDY) {
		if(buffer_is_empty(&term_tx) == false)
			usart_enable_interrupt(CONSOLE_UART, US_IER_TXRDY);
	}
    */

    //if there are bytes to be processed in the command buffer
    while(queue_is_empty(&cmd_buf) == false)
    {
        queue_remove_blocking(&cmd_buf, &c);
		if(c == '\r')
        {
			//printf("\n");
			command[cmd_len] = 0x00;
			process_command(command, &cmd_len);
		}
		else if((c == 8) || (c == 127))
        { //handle backspace
			command[cmd_len] = 0x00;
			if(cmd_len > 0) {
				cmd_len--;
				command[cmd_len] = 0x00;
			}
		}
		else
        {
			command[cmd_len++] = c;
        }
	}
}		




