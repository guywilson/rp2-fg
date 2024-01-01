#include "pico/stdlib.h"

#ifndef __INCL_GPIO_DEF
#define __INCL_GPIO_DEF

#define I2C0_SDA_ALT_PIN			16
#define I2C0_SLK_ALT_PIN			17

#define I2C1_SDA_ALT_PIN			14
#define I2C1_SLK_ALT_PIN			15

#define SQUARE_WAVE_OUT_PIN         21
#define DEBUG_ENABLE_PIN            22

#define DAC_PIN_D11                  2      // MSB
#define DAC_PIN_D10                  3
#define DAC_PIN_D9                   4
#define DAC_PIN_D8                   5
#define DAC_PIN_D7                   6
#define DAC_PIN_D6                   7
#define DAC_PIN_D5                   8
#define DAC_PIN_D4                   9
#define DAC_PIN_D3                  10
#define DAC_PIN_D2                  11
#define DAC_PIN_D1                  12
#define DAC_PIN_D0                  13      // LSB

#define DAC_BUS_MASK                (DAC_PIN_D11 |  \
                                     DAC_PIN_D10 |  \
                                     DAC_PIN_D9 |   \
                                     DAC_PIN_D8 |   \
                                     DAC_PIN_D7 |   \
                                     DAC_PIN_D6 |   \
                                     DAC_PIN_D5 |   \
                                     DAC_PIN_D4 |   \
                                     DAC_PIN_D3 |   \
                                     DAC_PIN_D2 |   \
                                     DAC_PIN_D1 |   \
                                     DAC_PIN_D0)

#define ONBAORD_LED_PIN             PICO_DEFAULT_LED_PIN

#endif
