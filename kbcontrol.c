
#include <stdio.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "terminal.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "bsp/board.h"
#include "tusb.h"

// Interface index depends on the order in configuration descriptor
enum {
  ITF_KEYBOARD = 0,
  ITF_MOUSE = 1
};

//reset_usb_boot(0,0);
void led_task(void *param);
void term_task(void *param);
void scan_task(void *param);
void hid_task(void *param);

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

    TaskHandle_t gScanTask = NULL;

    status = xTaskCreate(
                        scan_task,
                        "Scan Task",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gScanTask);

    TaskHandle_t gHidTask = NULL;

    status = xTaskCreate(
                        hid_task,
                        "HID Task",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gHidTask);

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

    print("Led Blink, FreeRTOS, USB \r\n");

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
//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
    print("USB Mounted\r\n");
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
    print("USB Unmounted\r\n");
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
    (void) remote_wakeup_en;
    print("USB Suspended\r\n");
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
    print("USB Remounted\r\n");
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

void hid_task(void *param)
{
    tusb_init();

    // init device stack on configured roothub port
    tud_init(BOARD_TUD_RHPORT);

    uint32_t btn = 0;

    while(1)
    {
        tud_task();

        btn++;

        // Remote wakeup
        if ( tud_suspended() && btn )
        {
            // Wake up host if we are in suspend mode
            // and REMOTE_WAKEUP feature is enabled by host
            tud_remote_wakeup();
        }

        /*------------- Keyboard -------------*/
        if ( tud_hid_n_ready(ITF_KEYBOARD) )
        {
            // use to avoid send multiple consecutive zero report for keyboard
            static bool has_key = false;

            if ( btn == 500 )
            {
                uint8_t keycode[6] = { 0 };
                keycode[0] = HID_KEY_A;
                keycode[1] = HID_KEY_B;
                keycode[2] = HID_KEY_C;

                tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, keycode);

                has_key = true;
            }else
            {
                // send empty key report if previously has key pressed
                if (has_key)
                    tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, NULL);
                has_key = false;
            }
        }

        /*------------- Mouse -------------*/
        if ( tud_hid_n_ready(ITF_MOUSE) )
        {
            if ( btn > 950 )
            {
                int8_t const delta = 5;

                // no button, right + down, no scroll pan
                tud_hid_n_mouse_report(ITF_MOUSE, 0, 0x00, delta, delta, 0, 0);

                if ( btn == 1000 )
                    btn = 0;
            }
        }

        // Poll every 10ms
        vTaskDelay(10);
    }
}


// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t* buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t itf, uint8_t report_id, hid_report_type_t report_type, uint8_t const* buffer, uint16_t bufsize)
{
  // TODO set LED based on CAPLOCK, NUMLOCK etc...
  (void) itf;
  (void) report_id;
  (void) report_type;
  (void) buffer;
  (void) bufsize;
}
