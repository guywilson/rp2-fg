#include <stdint.h>
#include <stdbool.h>

#include "hardware/i2c.h"
#include "i2c_rp2040.h"
#include "i2c_addr.h"
#include "MCP4725.h"

int setupMCP4725(i2c_inst_t * i2c) {
    return 0;
}

int writeSample(i2c_inst_t * i2c, uint16_t sample) {
    uint8_t         msg[3];

    msg[0] = MCP4725_CMD_WRITEDAC;

    msg[1] = (uint8_t)(sample >> 4);
    msg[2] = (uint8_t)((sample & 0x000F) << 4);

    return i2c_write_timeout_us(i2c, MCP4725_ADDRESS, msg, 3, false, 250);
}

int writeSampleFast(i2c_inst_t * i2c, uint16_t sample, bool isLast) {
    uint8_t         msg[2];

    msg[0] = MCP4725_CMD_WRITEDAC_FAST | (uint8_t)(sample >> 8);
    msg[1] = (uint8_t)(sample & 0x00FF);

    return i2c_write_timeout_us(i2c, MCP4725_ADDRESS, msg, 2, isLast, 250);
}
