#include "hardware/i2c.h"

#ifndef __INCL_MCP4725
#define __INCL_MCP4725

#define MCP4725_CMD_WRITEDAC            0x40
#define MCP4725_CMD_WRITEDAC_FAST       0x00

int setupMCP4725(i2c_inst_t * i2c);
int writeSample(i2c_inst_t * i2c, uint16_t sample);
int writeSampleFast(i2c_inst_t * i2c, uint16_t sample, bool isFirst, bool isLast);

#endif
