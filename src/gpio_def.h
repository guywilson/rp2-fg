#include "pico/stdlib.h"

#ifndef __INCL_GPIO_DEF
#define __INCL_GPIO_DEF

#define I2C0_SDA_ALT_PIN			16
#define I2C0_SLK_ALT_PIN			17

#define I2C1_SDA_ALT_PIN			14
#define I2C1_SLK_ALT_PIN			15

#define CORE1_DEBUG_PIN             20
#define SQUARE_WAVE_OUT_PIN         21
#define DEBUG_ENABLE_PIN            22

#define DAC_PIN_D0                   2      // LSB
#define DAC_PIN_D1                   3
#define DAC_PIN_D2                   4
#define DAC_PIN_D3                   5
#define DAC_PIN_D4                   6
#define DAC_PIN_D5                   7
#define DAC_PIN_D6                   8
#define DAC_PIN_D7                   9
#define DAC_PIN_D8                  10
#define DAC_PIN_D9                  11
#define DAC_PIN_D10                 12
#define DAC_PIN_D11                 13      // MSB

#define DAC_BUS_MASK               ((1 << DAC_PIN_D11)  |   \
                                    (1 << DAC_PIN_D10)  |   \
                                    (1 << DAC_PIN_D9)   |   \
                                    (1 << DAC_PIN_D8)   |   \
                                    (1 << DAC_PIN_D7)   |   \
                                    (1 << DAC_PIN_D6)   |   \
                                    (1 << DAC_PIN_D5)   |   \
                                    (1 << DAC_PIN_D4)   |   \
                                    (1 << DAC_PIN_D3)   |   \
                                    (1 << DAC_PIN_D2)   |   \
                                    (1 << DAC_PIN_D1)   |   \
                                    (1 << DAC_PIN_D0))

#define ONBAORD_LED_PIN             PICO_DEFAULT_LED_PIN

#endif
