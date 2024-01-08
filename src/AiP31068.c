#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "pico/stdlib.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "hardware/timer.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "i2c_addr.h"
#include "gpio_def.h"
#include "AiP31068.h"

#define STATE_BEGIN             0x0010
#define STATE_FUNCTION_SET      0x0020
#define STATE_DISPLAY_CTRL      0x0030
#define STATE_CURSOR_DISPLAY    0x0035
#define STATE_CLEAR             0x0040
#define STATE_SET_ENTRY_MODE    0x0050
#define STATE_TURN_ON           0x0060
#define STATE_PRINT_WELCOME     0x0070

#define NUM_FUNCTION_SET_TRIES  1

void taskLCDInitialise(PTASKPARM p) {
    static int          state = STATE_BEGIN;
    static const char * message = "Bloody Hell!                            It works!";
    static int          i = 0;
    rtc_t               delay;
    i2c_inst_t *        i2c = (i2c_inst_t *)p;

    switch (state) {
        case STATE_BEGIN:
            i2c_init(i2c, 100000);

            gpio_set_function(I2C0_SDA_ALT_PIN, GPIO_FUNC_I2C);
            gpio_set_function(I2C0_SLK_ALT_PIN, GPIO_FUNC_I2C);

            state = STATE_FUNCTION_SET;
            delay = rtc_val_ms(50);
            i = 0;
            break;

        case STATE_FUNCTION_SET:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_FUNCTION_SET | 
                        CMD_FUNCTION_SET_2_LINE | 
                        CMD_FUNCTION_SET_FONTSIZE_5x8);

            i++;

            if (i < NUM_FUNCTION_SET_TRIES) {
                state = STATE_FUNCTION_SET;
            }
            else {
                state = STATE_DISPLAY_CTRL;
                i = 0;
            }
            
            delay = rtc_val_ms(5);
            break;

        case STATE_DISPLAY_CTRL:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_DISPLAY_CTRL | 
                        CMD_DISPLAY_CTRL_DISPLAY_ON | 
                        CMD_DISPLAY_CTRL_CURSOR_ON |
                        CMD_DISPLAY_CTRL_CURSOR_BLINK_ON);

            state = STATE_CURSOR_DISPLAY;
            delay = rtc_val_ms(1);
            break;

        case STATE_CURSOR_DISPLAY:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_CURSOR_DISPLAY | 
                        CMD_CURSOR_DISPLAY_SHIFT_SCROLL | 
                        CMD_CURSOR_DISPLAY_SHIFT_RIGHT);

            state = STATE_CLEAR;
            delay = rtc_val_ms(1);
            break;

        case STATE_CLEAR:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_CLEAR_DISPLAY);

            state = STATE_SET_ENTRY_MODE;
            delay = rtc_val_ms(1);
            break;

        case STATE_SET_ENTRY_MODE:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_ENTRY_MODE_SET | 
                        CMD_ENTRY_MODE_AUTOSCROLL_OFF | 
                        CMD_ENTRY_MODE_CURSOR_INC);

            state = STATE_PRINT_WELCOME;
            delay = rtc_val_ms(1);
            break;
            // return;

        case STATE_TURN_ON:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_DISPLAY_CTRL | 
                        CMD_DISPLAY_CTRL_DISPLAY_ON | 
                        CMD_DISPLAY_CTRL_CURSOR_ON |
                        CMD_DISPLAY_CTRL_CURSOR_BLINK_ON);

            state = STATE_PRINT_WELCOME;
            delay = rtc_val_ms(1);
            break;

        case STATE_PRINT_WELCOME:
            if (i < strlen(message)) {
                lcdWriteChar(i2c, message[i++]);
                delay = rtc_val_ms(1);
                break;
            }

            return;
    }

    scheduleTask(TASK_LCD_INITIALISE, delay, false, p);
}

int lcdSetup_AIP31068(i2c_inst_t * i2c) {
    scheduleTask(
            TASK_LCD_INITIALISE, 
            rtc_val_ms(2), 
            false, 
            i2c);

    return 0;
}

int lcdWriteCommand(i2c_inst_t * i2c, uint8_t cmd) {
    uint8_t         msg[2];
    int             error;

    msg[0] = 0x00;
    msg[1] = cmd;

    error =  i2c_write_timeout_us(i2c, AIP31068_LCD_ADDRESS >> 1, msg, 2, false, 250);

    busy_wait_us(50);

    return error;
}

int lcdWriteChar(i2c_inst_t * i2c, char ch) {
    uint8_t         msg[2];
    int             error;

    msg[0] = 0x40;
    msg[1] = (uint8_t)ch;

    error =  i2c_write_timeout_us(i2c, AIP31068_LCD_ADDRESS >> 1, msg, 2, false, 250);
    
    busy_wait_us(50);

    return error;
}

void lcdPrint(i2c_inst_t * i2c, const char * str) {
    int         len = strlen(str);
    int         i;

    for (i = 0;i < len;i++) {
        lcdWriteChar(i2c, str[i]);
    }
}
