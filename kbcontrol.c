
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "terminal.h"


int main()
{
    stdio_init_all();
    const uint LED_PIN = PICO_DEFAULT_LED_PIN;
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    term_init();

    print("Hello, world!\r\n");
    while(1)
    {
        gpio_put(LED_PIN, 1);
        sleep_ms(200);
        gpio_put(LED_PIN, 0);
        sleep_ms(200);


        terminal_task();

    }
}


