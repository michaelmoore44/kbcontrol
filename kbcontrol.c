
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "terminal.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"


void led_task(void *param);
void term_task(void *param);
void scan_task(void *param);

#define NUM_ROWS 6
#define NUM_COLS 8

uint8_t keys[NUM_ROWS * NUM_COLS];
uint8_t keys_count[NUM_ROWS * NUM_COLS];
uint8_t row = 0;
uint8_t col = 0;
uint8_t current_state = 0;
uint8_t scan_idx;

int main()
{
    stdio_init_all();

    TaskHandle_t gLEDTask = NULL;

    uint32_t status = xTaskCreate(
                        led_task,
                        "Led Task",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gLEDTask);

    TaskHandle_t gTermTask = NULL;

    status = xTaskCreate(
                        term_task,
                        "Terminal Task",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gTermTask);

    status = xTaskCreate(
                        scan_task,
                        "Scan Task",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gTermTask);
    
    vTaskStartScheduler();

    //should never get here
    for (;;)
    {
    }
}

void led_task(void *param)
{
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    while(true)
    {   
        gpio_put(LED_PIN, 1);
        vTaskDelay(200);
        gpio_put(LED_PIN, 0);
        vTaskDelay(200);
    }
}

void term_task(void *param)
{
    term_init();

    print("Led Blink with FreeRTOS \r\n");

    while(true)
    {   
        terminal_task();
        vTaskDelay(10);
    }
}

void scan_task(void *param)
{
    const uint R0_PIN = 21;
    const uint R1_PIN = 20;
    const uint R2_PIN = 19;
    const uint R3_PIN = 18;
    const uint R4_PIN = 17;
    const uint R5_PIN = 16;

    const uint C0_PIN = 8;
    const uint C1_PIN = 9;
    const uint C2_PIN = 10;
    const uint C3_PIN = 11;
    const uint C4_PIN = 12;
    const uint C5_PIN = 13;
    const uint C6_PIN = 14;
    const uint C7_PIN = 15;

    //Set all rows as outputs
    gpio_init(R0_PIN);
    gpio_set_dir(R0_PIN, GPIO_OUT);
    gpio_init(R1_PIN);
    gpio_set_dir(R1_PIN, GPIO_OUT);
    gpio_init(R2_PIN);
    gpio_set_dir(R2_PIN, GPIO_OUT);
    gpio_init(R3_PIN);
    gpio_set_dir(R3_PIN, GPIO_OUT);
    gpio_init(R4_PIN);
    gpio_set_dir(R4_PIN, GPIO_OUT);
    gpio_init(R5_PIN);
    gpio_set_dir(R5_PIN, GPIO_OUT);

    //Set all columns as inputs
    gpio_init(C0_PIN);
    gpio_set_dir(C0_PIN, GPIO_IN);
    gpio_pull_down(C0_PIN);
    gpio_init(C1_PIN);
    gpio_set_dir(C1_PIN, GPIO_IN);
    gpio_pull_down(C1_PIN);
    gpio_init(C2_PIN);
    gpio_set_dir(C2_PIN, GPIO_IN);
    gpio_pull_down(C2_PIN);
    gpio_init(C3_PIN);
    gpio_set_dir(C3_PIN, GPIO_IN);
    gpio_pull_down(C3_PIN);
    gpio_init(C4_PIN);
    gpio_set_dir(C4_PIN, GPIO_IN);
    gpio_pull_down(C4_PIN);
    gpio_init(C5_PIN);
    gpio_set_dir(C5_PIN, GPIO_IN);
    gpio_pull_down(C5_PIN);
    gpio_init(C6_PIN);
    gpio_set_dir(C6_PIN, GPIO_IN);
    gpio_pull_down(C6_PIN);
    gpio_init(C7_PIN);
    gpio_set_dir(C7_PIN, GPIO_IN);
    gpio_pull_down(C7_PIN);

    gpio_put(R0_PIN, 0);
    gpio_put(R1_PIN, 0);
    gpio_put(R2_PIN, 0);
    gpio_put(R3_PIN, 0);
    gpio_put(R4_PIN, 0);
    gpio_put(R5_PIN, 0);

    gpio_put(R0_PIN, 1);
    gpio_put(R5_PIN, 0);

    while(true)
    {
        //scan all the columns for this row
        for(col = 0; col < NUM_COLS; col++)
        {
            scan_idx = NUM_ROWS * col + row;
            if(col == 0)current_state = (gpio_get(C0_PIN) ? 0x01 : 0x00);
            else if(col == 1)current_state = (gpio_get(C1_PIN) ? 0x01 : 0x00);
            else if(col == 2)current_state = (gpio_get(C2_PIN) ? 0x01 : 0x00);
            else if(col == 3)current_state = (gpio_get(C3_PIN) ? 0x01 : 0x00);
            else if(col == 4)current_state = (gpio_get(C4_PIN) ? 0x01 : 0x00);
            else if(col == 5)current_state = (gpio_get(C5_PIN) ? 0x01 : 0x00);
            else if(col == 6)current_state = (gpio_get(C6_PIN) ? 0x01 : 0x00);
            else if(col == 7)current_state = (gpio_get(C7_PIN) ? 0x01 : 0x00);

            if(current_state != keys[scan_idx])
            {
                keys_count[scan_idx]++;
                if(keys_count[scan_idx] > 6)
                {
                    keys[scan_idx] = current_state;
                    print("Key %u", scan_idx);
                    if(current_state == 0x01)
                    {
                        print(" p ");
                    }
                    else if(current_state == 0x00)
                    {
                        print(" r ");
                    }
                    else
                    {
                        print(" ? ");
                    }
                    print("\r\n");
                }
            }
            else
            {
                keys_count[scan_idx] = 0;
            }
        }
        //advance to the next row
        row++;
        if(row >= NUM_ROWS)
        {
            //start over with the first row
            row = 0;
        }

        //clear the row, and set the next one
        if(row == 0)
        {
            gpio_put(R0_PIN, 1);
            gpio_put(R5_PIN, 0);
        }
        else if(row == 1)
        {
            gpio_put(R1_PIN, 1);
            gpio_put(R0_PIN, 0);
        }
        else if(row == 2)
        {
            gpio_put(R2_PIN, 1);
            gpio_put(R1_PIN, 0);
        }
        else if(row == 3)
        {
            gpio_put(R3_PIN, 1);
            gpio_put(R2_PIN, 0);
        }
        else if(row == 4)
        {
            gpio_put(R4_PIN, 1);
            gpio_put(R3_PIN, 0);
        }
        else if(row == 5)
        {
            gpio_put(R5_PIN, 1);
            gpio_put(R4_PIN, 0);
        }

        vTaskDelay(1);
    }
}

