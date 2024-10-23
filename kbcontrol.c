
#include <stdio.h>
#include <string.h>
#include "hardware/gpio.h"
#include "pico/stdlib.h"

#include "terminal.h"
#include "split.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "task.h"

#include "bsp/board.h"
#include "tusb.h"

#include "keycode.h"

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
void split_tsk(void *param);
void application_task(void *param);

volatile bool bChange = false;
volatile bool bPressed = false;

uint8_t keycode[6] = { 0 };

#define NUM_ROWS 6
#define NUM_COLS 8
#define NUM_COMBINED_COLS (NUM_COLS * 2)

uint8_t keys[NUM_ROWS * NUM_COLS];
uint8_t keys_count[NUM_ROWS * NUM_COLS];
uint8_t row = 0;
uint8_t col = 0;
uint8_t current_state = 0;
uint8_t scan_idx;

queue_t* split_keys_buf;
uint8_t split_keys[NUM_ROWS * NUM_COLS];
#define NUM_SPLIT_KEYS (NUM_ROWS * NUM_COLS)
#define NUM_COMBINED_KEYS (TU_ARRAY_SIZE(keys) * 2)

#define NUM_LAYERS (2)

uint8_t keys_combined[NUM_COMBINED_KEYS];

/*
uint8_t layer[TU_ARRAY_SIZE(keys)] = {\
KC_NO  , KC_6   , KC_7   , KC_8   , KC_9   , KC_NO  , KC_NO  , KC_NO  ,\
KC_LGUI, KC_Y   , KC_U   , KC_I   , KC_O   , KC_0   , KC_MINS, KC_EQL ,\
KC_LGUI, KC_H   , KC_J   , KC_K   , KC_L   , KC_P   , KC_LBRC, KC_RBRC,\
KC_LGUI, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SCLN, KC_QUOT, KC_ESC ,\
KC_SPC , KC_RALT, KC_NO  , KC_DEL , KC_BSPC, KC_SLSH, KC_BSLS, KC_ENT ,\
KC_LSFT, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,\
};*/



/*
(0xB0-DD) are not used, so I am going to use them as codes for eg. function or leader keys

mouse wheel up - KC_WH_U
mouse wheel down - KC_WH_D

todo:
make the none keys 7x_   _______
*/


#define _______ (KC_NO)
#define K_FUNC  (0xB0)
//#define K_LEADR  (0xB1)


//                                                                      |
uint8_t layer[TU_ARRAY_SIZE(keys) * 2 * NUM_LAYERS] = {
_______, _______, _______, KC_2   , KC_3   , KC_4   , KC_5   , _______,   _______, KC_6   , KC_7   , KC_8   , KC_9   , _______, _______, _______,\
KC_ESC , KC_GRV , KC_1   , KC_W   , KC_E   , KC_R   , KC_T   , KC_ESC ,   KC_ESC , KC_Y   , KC_U   , KC_I   , KC_O   , KC_0   , KC_MINS, KC_EQL ,\
KC_LGUI, KC_TAB , KC_Q   , KC_S   , KC_D   , KC_F   , KC_G   , KC_LGUI,   KC_LGUI, KC_H   , KC_J   , KC_K   , KC_L   , KC_P   , KC_LBRC, KC_RBRC,\
KC_ENT , KC_NO  , KC_A   , KC_X   , KC_C   , KC_V   , KC_B   , KC_ENT ,   KC_LGUI, KC_N   , KC_M   , KC_COMM, KC_DOT , KC_SCLN, KC_QUOT, KC_ESC ,\
KC_ENT , KC_ESC , KC_Z   , KC_BSPC, KC_DEL , KC_N   , KC_LCTL, K_FUNC ,   KC_SPC , KC_RALT, _______, KC_DEL , KC_BSPC, KC_SLSH, KC_BSLS, KC_ENT ,\
_______, _______, _______, _______, _______, _______, _______, KC_LSFT,   KC_RSFT, _______, _______, _______, _______, _______, _______, _______,\
_______, _______, _______, KC_F2  , KC_F3  , KC_F4  , KC_F5  , _______,   _______, KC_F6  , KC_F7  , KC_F8  , KC_F9  , _______, _______, _______,\
_______, _______, KC_F1  , _______, _______, _______, _______, _______,   _______, _______, KC_HOME, _______, KC_END , KC_F10 , _______, _______,\
_______, _______, _______, KC_WH_U, KC_WH_D, _______, _______, _______,   _______, KC_LEFT, KC_DOWN, KC_UP  , KC_RGHT, _______, _______, _______,\
_______, _______, _______, _______, _______, _______, _______, _______,   _______, _______, _______, _______, _______, _______, _______, _______,\
_______, _______, _______, _______, _______, _______, _______, _______,   _______, _______, _______, _______, _______, _______, _______, _______,\
_______, _______, _______, _______, _______, _______, _______, _______,   _______, _______, _______, _______, _______, _______, _______, _______,\
};//                                                                    |
//                                                                      |

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
                        4096,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gHidTask);

    TaskHandle_t gSplitTask = NULL;
    status = xTaskCreate(
                        split_tsk,
                        "Split Task",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gSplitTask);

    TaskHandle_t gAppTask = NULL;
    status = xTaskCreate(
                        application_task,
                        "Application Task",
                        1024,
                        NULL,
                        tskIDLE_PRIORITY,
                        &gAppTask);
    
    vTaskStartScheduler();

    //should never get here
    for (;;)
    {
    }
}

void set_sent(uint8_t* data)
{
    *data |= (1<<1);
}

bool sent(uint8_t data)
{
    bool ret_val = false;
    if((data >> 1) & 1)
        ret_val = true;

    return ret_val;
}

bool not_sent(uint8_t data)
{
    bool ret_val = true;
    if(sent(data))
        ret_val = false;

    return ret_val;
}

void combine_split(void)
{
    uint8_t row, col;
    uint8_t code_count = 0;
    uint8_t idx, j;

    for(row = 0; row < NUM_ROWS; row++)
    {
        for(col = 0; col < NUM_COMBINED_COLS; col++)
        {
            idx = NUM_COMBINED_COLS * row + col;
            if(col >= NUM_COLS) // if controller half
            {
                j = NUM_COLS * row + (col - NUM_COLS);
                if((keys_combined[idx] != keys[j]) && not_sent(keys_combined[idx]))
                {
                    if(keys[j] == 1)
                        print("coKey %u,%u p\r\n", idx, j);
                    else
                        print("coKey %u,%u r\r\n", idx, j);
                }
                
                //don't overwrite to pressed if the key has already been sent
                if(not_sent(keys_combined[idx]) || (keys[j] == 0x00))
                {
                    keys_combined[idx] = keys[j];
                }
            }
            else //if peripheral half
            {
                j = NUM_COLS * row + col;
                if((keys_combined[idx] != split_keys[j]) && not_sent(keys_combined[idx]))
                {
                    if(split_keys[j] == 1)
                        print("coKey %u,%u p\r\n", idx, j);
                    else
                        print("coKey %u,%u r\r\n", idx, j);
                }
                
                //don't overwrite to pressed if the key has already been sent
                if(not_sent(keys_combined[idx]) || (split_keys[j] == 0x00))
                {
                    keys_combined[idx] = split_keys[j];
                }
            }
        }
    }
}

void apply_layers(uint8_t* key_state, uint8_t* codes, uint8_t* mods)
{
    //memset(codes, 0, sizeof(keys));
    uint8_t row, col;
    uint8_t code_count = 0;
    uint8_t idx;
    memset(codes, 0x00, 6);
    *mods = 0x00;

    bool fn_pressed = false;

    //scan all keys to determine if function has been pressed
    for(row = 0; row < NUM_ROWS; row++)
    {
        for(col = 0; col < NUM_COMBINED_COLS; col++)
        {
            idx = NUM_COMBINED_COLS * row + col;
            if((key_state[idx] & 1) && (layer[idx] == K_FUNC))
            {
                fn_pressed = true;
                print("Fn Pressed\r\n");
            }
        }
    }

    //determine which keys should be sent
    for(row = 0; row < NUM_ROWS; row++)
    {
        for(col = 0; col < NUM_COMBINED_COLS; col++)
        {
            idx = NUM_COMBINED_COLS * row + col;
            //print(" %02u,%02u-%02u,%02u\r\n", row, col, idx, key_state[idx]);
            if(key_state[idx] & 1)
            {
                //if function is pressed, and fnlayer is a key, then add code from fn layer
                if((fn_pressed == true) && (IS_KEY(layer[idx + NUM_COMBINED_KEYS])))
                {
                    print("Applying Fn Layer: idx: %u, code 0x%02X\r\n", (idx + NUM_COMBINED_KEYS), layer[idx + NUM_COMBINED_KEYS]);
                    if(not_sent(key_state[idx]))
                    {
                        codes[code_count++] = layer[idx + NUM_COMBINED_KEYS];
                        set_sent(&key_state[idx]);
                    }
                    else
                    {
                        print("already sent\r\n");
                    }

                }
                else
                {
                    print("Applying Layer: idx: %u, cc %u\r\n", idx, code_count);
                    //check for modifier 
                    if(IS_MOD(layer[idx]))
                    {
                        if(layer[idx] == KC_LCTRL)
                            *mods |= KEYBOARD_MODIFIER_LEFTCTRL;
                        else if(layer[idx] == KC_LSHIFT)
                            *mods |= KEYBOARD_MODIFIER_LEFTSHIFT;
                        else if(layer[idx] == KC_LALT)
                            *mods |= KEYBOARD_MODIFIER_LEFTALT;
                        else if(layer[idx] == KC_LGUI)
                            *mods |= KEYBOARD_MODIFIER_LEFTGUI;
                        else if(layer[idx] == KC_RCTRL)
                            *mods |= KEYBOARD_MODIFIER_RIGHTCTRL;
                        else if(layer[idx] == KC_RSHIFT)
                            *mods |= KEYBOARD_MODIFIER_RIGHTSHIFT;
                        else if(layer[idx] == KC_RALT)
                            *mods |= KEYBOARD_MODIFIER_RIGHTALT;
                        else if(layer[idx] == KC_RGUI)
                            *mods |= KEYBOARD_MODIFIER_RIGHTGUI;
                    }
                    else
                    {
                        if(not_sent(key_state[idx]))
                        {
                            if(IS_KEY(layer[idx]))//only send if it is an actual key
                            {
                                codes[code_count++] = layer[idx];
                                set_sent(&key_state[idx]);
                            }
                        }
                        else
                        {
                            print("already sent\r\n");
                        }
                    }
                }

                for(int i=0; i < code_count; i++)
                {
                    print(" %u,", codes[i]);
                }
                print("\r\n");
            }
        }
    }
    //print("\r\n\r\nend..");
}

void application_task(void *param)
{
    uint8_t tx_msg[256];
    uint8_t tx_msg_len;
    uint8_t level;
    uint8_t idx;
    uint8_t ch;
    uint8_t modifiers;

    vTaskDelay(1000);

    while(true)
    {   
        //if all split keys have been received
        level = (uint8_t)queue_get_level(split_keys_buf);
        if(level >= NUM_SPLIT_KEYS)
        {
            //read num split keys
            for(idx = 0; idx < NUM_SPLIT_KEYS; idx++)
            {
                queue_try_remove(split_keys_buf, &ch);
                split_keys[idx] = ch;
            }
            bChange = true;
        }


        /*------------- Keyboard -------------*/
        if ( tud_hid_n_ready(ITF_KEYBOARD) )
        {
            if ( bChange == true )
            {
                combine_split();
                print("Applying Layers\r\n");
                apply_layers(keys_combined, keycode, &modifiers);

                if(modifiers > 0x00 || keycode[0] > 0x00)
                {
                    print("Sending: ");
                    uint8_t index = 0;
                    while (keycode[index] != 0x00)
                    {
                        print(" 0x%02X", keycode[index]);
                        index++;
                    }
                    print("\r\n");
                    tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, modifiers, keycode);
                    print("*Sent: pressed keys sent\r\n");
                }
                else
                {
                    tud_hid_n_keyboard_report(ITF_KEYBOARD, 0, 0, NULL);
                    print("*Sent: All keys released, none sent 1\r\n\r\n\r\n");
                }
                bChange = false;
            }
        }

        /*------------- Mouse -------------*/
        /*
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
        */

        vTaskDelay(10);
    }
}

void split_tsk(void *param)
{
    split_init();
    split_keys_buf = split_get_keys_buf();

    while(true)
    {   
        split_task();
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

void term_task(void *param)
{
    term_init();

    print("\r\n\r\nUSB Keyboard and Mouse, Controller, v0.0.1\r\n");

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

    row = 0;
    col = 0;

    while(true)
    {
        //scan all the columns for this row
        for(col = 0; col < NUM_COLS; col++)
        {
            scan_idx = (NUM_COLS * row) + col;
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
                    if(current_state == 0x01)
                    {
                        print("\r\n");
                    }
                    print("scKey %u", scan_idx);
                    if(current_state == 0x01)
                    {
                        print(" p ");
                        bPressed = true;
                    }
                    else if(current_state == 0x00)
                    {
                        print(" r ");
                        bPressed = false;
                    }
                    else
                    {
                        print(" ? ");
                    }
                    print("\r\n");

                    bChange = true;
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

            //check for a change. if there was a change, take a snapshot and indicate change
        }

        //Set the row output pins
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

    while(1)
    {
        tud_task();


        // Remote wakeup
        if ( tud_suspended())
        {
            // Wake up host if we are in suspend mode
            // and REMOTE_WAKEUP feature is enabled by host
            tud_remote_wakeup();
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
