#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "scheduler.h"
#include "taskdef.h"
#include "i2c_rp2040.h"
#include "rtc_rp2040.h"
#include "i2c_addr.h"
#include "gpio_def.h"
#include "AiP31068.h"

#define STATE_BEGIN             0x0010
#define STATE_FUNCTION_SET_1    0x0020
#define STATE_FUNCTION_SET_2    0x0021
#define STATE_FUNCTION_SET_3    0x0022
#define STATE_TURN_OFF          0x0030
#define STATE_CLEAR             0x0040
#define STATE_SET_ENTRY_MODE    0x0050

void taskLCDInitialise(PTASKPARM p) {
    static int          state = STATE_BEGIN;
    rtc_t               delay;
    i2c_inst_t *        i2c = (i2c_inst_t *)p;

    switch (state) {
        case STATE_BEGIN:
            i2c_init(i2c, 100000);

            gpio_set_function(I2C0_SDA_ALT_PIN, GPIO_FUNC_I2C);
            gpio_set_function(I2C0_SLK_ALT_PIN, GPIO_FUNC_I2C);

            state = STATE_FUNCTION_SET_1;
            delay = rtc_val_ms(50);
            break;

        case STATE_FUNCTION_SET_1:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_FUNCTION_SET | 
                        CMD_FUNCTION_SET_2_LINE | 
                        CMD_FUNCTION_SET_FONTSIZE_5x8);

            state = STATE_FUNCTION_SET_2;
            delay = rtc_val_ms(5);
            break;

        case STATE_FUNCTION_SET_2:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_FUNCTION_SET | 
                        CMD_FUNCTION_SET_2_LINE | 
                        CMD_FUNCTION_SET_FONTSIZE_5x8);

            state = STATE_FUNCTION_SET_3;
            delay = rtc_val_ms(1);
            break;

        case STATE_FUNCTION_SET_3:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_FUNCTION_SET | 
                        CMD_FUNCTION_SET_2_LINE | 
                        CMD_FUNCTION_SET_FONTSIZE_5x8);

            state = STATE_TURN_OFF;
            delay = rtc_val_ms(2);
            break;

        case STATE_TURN_OFF:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_DISPLAY_CTRL | 
                        CMD_DISPLAY_CTRL_DISPLAY_OFF | 
                        CMD_DISPLAY_CTRL_CURSOR_OFF);

            state = STATE_CLEAR;
            delay = rtc_val_ms(2);
            break;

        case STATE_CLEAR:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_CLEAR_DISPLAY);

            state = STATE_SET_ENTRY_MODE;
            delay = rtc_val_ms(2);
            break;

        case STATE_SET_ENTRY_MODE:
            lcdWriteCommand(
                        i2c, 
                        AIP31068_CMD_ENTRY_MODE_SET | 
                        CMD_ENTRY_MODE_AUTOSCROLL_OFF | 
                        CMD_ENTRY_MODE_CURSOR_INC);

            /*
            ** Finish here...
            */
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

    rtcDelay(50);

    return error;
}

int lcdWriteChar(i2c_inst_t * i2c, char ch) {
    uint8_t         msg[2];
    int             error;

    msg[0] = 0x01;
    msg[1] = (uint8_t)ch;

    error =  i2c_write_timeout_us(i2c, AIP31068_LCD_ADDRESS >> 1, msg, 2, false, 250);
    
    return error;
}

void lcdPrint(i2c_inst_t * i2c, char * str) {
    int         len = strlen(str);
    int         i;

    for (i = 0;i < len;i++) {
        lcdWriteChar(i2c, str[i]);
        rtcDelay(50);
    }
}
