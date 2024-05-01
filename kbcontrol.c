
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "terminal.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"


void led_task(void *param);
void term_task(void *param);

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
    
    vTaskStartScheduler();

    //should never get here
    for (;;)
    {
    }
}

void term_task(void *param)
{
    term_init();

    print("Led Blink with FreeRTOS\r\n");

    while(true)
    {   
        terminal_task();
        vTaskDelay(10);
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


