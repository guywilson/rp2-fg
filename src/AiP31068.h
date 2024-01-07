#include "hardware/i2c.h"

#ifndef __INCL_AIP31068
#define __INCL_AIP31068

#define AIP31068_CMD_CLEAR_DISPLAY              0x01
#define AIP31068_CMD_RETURN_HOME                0x02
#define AIP31068_CMD_ENTRY_MODE_SET             0x04
#define AIP31068_CMD_DISPLAY_CTRL               0x08
#define AIP31068_CMD_CURSOR_DISPLAY             0x10
#define AIP31068_CMD_FUNCTION_SET               0x20
#define AIP31068_CMD_SET_CGRAM_ADDR             0x40
#define AIP31068_CMD_SET_DDRAM_ADDR             0x80

#define CMD_ENTRY_MODE_CURSOR_DEC               0x00
#define CMD_ENTRY_MODE_CURSOR_INC               0x02
#define CMD_ENTRY_MODE_AUTOSCROLL_OFF           0x00
#define CMD_ENTRY_MODE_AUTOSCROLL_ON            0x01

#define CMD_DISPLAY_CTRL_CURSOR_NO_BLINK        0x00
#define CMD_DISPLAY_CTRL_CURSOR_BLINK_ON        0x01
#define CMD_DISPLAY_CTRL_CURSOR_OFF             0x00
#define CMD_DISPLAY_CTRL_CURSOR_ON              0x02
#define CMD_DISPLAY_CTRL_DISPLAY_OFF            0x00
#define CMD_DISPLAY_CTRL_DISPLAY_ON             0x04

#define CMD_CURSOR_DISPLAY_SHIFT_LEFT           0x00
#define CMD_CURSOR_DISPLAY_SHIFT_RIGHT          0x04
#define CMD_CURSOR_DISPLAY_SHIFT_SHIFT          0x00
#define CMD_CURSOR_DISPLAY_SHIFT_SCROLL         0x08

#define CMD_FUNCTION_SET_FONTSIZE_5x8           0x00
#define CMD_FUNCTION_SET_FONTSIZE_5x10          0x04
#define CMD_FUNCTION_SET_1_LINE                 0x00
#define CMD_FUNCTION_SET_2_LINE                 0x08

void    taskLCDInitialise(PTASKPARM p);
int     lcdSetup_AIP31068(i2c_inst_t * i2c);
int     lcdWriteCommand(i2c_inst_t * i2c, uint8_t cmd);
int     lcdWriteChar(i2c_inst_t * i2c, char ch);
void    lcdPrint(i2c_inst_t * i2c, char * pszFmt);

#endif
