

#include "pico/stdlib.h"
#include "hardware/uart.h"
#include "hardware/irq.h"
#include "pico/util/queue.h"

#include "split.h"
#include "terminal.h"
#include "cobs.h"

#define UART_ID uart1
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

#define UART_TX_PIN 4
#define UART_RX_PIN 5

#define TX_BUF_SIZE (256)
#define RX_BUF_SIZE (256)

queue_t tx_buf;
uint8_t tx_buffer[TX_BUF_SIZE];

queue_t rx_buf;
static uint8_t rx_buffer[RX_BUF_SIZE];

static uint8_t rx_msg[RX_BUF_SIZE];
static uint8_t rx_msg_len;

static uint8_t enc_msg[TX_BUF_SIZE];
static uint8_t enc_msg_len;

static uint8_t dec_msg[RX_BUF_SIZE];
static uint8_t dec_msg_len;

// Uart interrupt handler
void on_uart1_interrupt()
{
	uint8_t ch;

	//RX
    while (uart_is_readable(UART_ID))
    {
        ch = uart_getc(UART_ID);
        queue_try_add(&rx_buf, &ch);
    }
	//TX
    while (uart_is_writable(UART_ID) && !queue_is_empty(&tx_buf))
    {
		if(queue_try_remove(&tx_buf, &ch))
		{
			uart_putc(UART_ID, ch);
		}
    }
	//clear all interrupts
	uart_get_hw(UART_ID)->icr = uart_get_hw(UART_ID)->mis;
}

void split_init(void)
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

    // Set uart data format
    uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);

    // Turn off FIFO's - we want to do this character by character
    uart_set_fifo_enabled(UART_ID, false);

    // Set up a RX interrupt
    // We need to set up the handler first
    // Select correct interrupt for the UART we are using
    int UART_IRQ = UART_ID == uart0 ? UART0_IRQ : UART1_IRQ;

    // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart1_interrupt);
    irq_set_enabled(UART_IRQ, true);

    // Now enable the UART to send interrupts
    uart_set_irq_enables(UART_ID, true, true);

	//initialize transmit and receive buffers
	queue_init(&rx_buf, sizeof(rx_buffer[0]), RX_BUF_SIZE);
	queue_init(&tx_buf, sizeof(tx_buffer[0]), TX_BUF_SIZE);
}

//send the raw data over the uart
void split_transmit(uart_inst_t* uart, char* dat, uint8_t len)
{
	uint8_t ch;
	uint8_t idx;

	for (idx = 0; idx < len; idx++)
	{
		queue_try_add(&tx_buf, dat);
		dat++;
	}
	//trigger uart interrupt here. 
    if(uart_is_writable(UART_ID))
    {
		if(queue_try_remove(&tx_buf, &ch))
		{
			uart_putc(UART_ID, ch);
		}
    }
}

void split_tx_msg(uint8_t* msg, uint8_t msg_len)
{
	enc_msg_len = cobsEncode(msg, msg_len, enc_msg);
	enc_msg[enc_msg_len] = 0x00;
	enc_msg_len++;
	split_transmit(UART_ID, enc_msg, enc_msg_len);
}
			

void process_rx_msg(uint8_t* msg, uint8_t msg_len)
{
	print("rx: %s \r\n", msg);
	dec_msg_len = 0x00;
}

void split_task(void)
{
	//uint32_t dw_status = usart_get_status(CONSOLE_UART);
	uint8_t c;

    //if there are bytes to be processed in the rx_buffer
    while(queue_is_empty(&rx_buf) == false)
    {
        queue_remove_blocking(&rx_buf, &c);
		rx_msg[rx_msg_len++] = c;
		//look for end
		if(c == 0x00)
        {
			rx_msg[rx_msg_len] = 0x00;
			dec_msg_len = cobsDecode(rx_msg, rx_msg_len, dec_msg);
			rx_msg_len = 0x00;
			process_rx_msg(dec_msg, dec_msg_len);
		}
	}
}		




