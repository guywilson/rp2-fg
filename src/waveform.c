#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"
#include "hardware/i2c.h"

#include "logger.h"
#include "rtc_rp2040.h"
#include "MCP4725.h"
#include "gpio_def.h"
#include "waveform.h"

//#define USE_MCP4725_DAC

#define PI                          3.1415926535
#define DEGREE_TO_RADIANS           (PI / 180.0)

#define radians(d)                  (d * DEGREE_TO_RADIANS)

static waveform_type_t              currentWT;

static volatile bool                doUpdate = false;
static volatile int                 sampleNum = 0;

static uint32_t                     numSamplesPerCycle = 0;

static uint16_t _samples[DAC_SAMPLE_RATE];

#ifndef USE_MCP4725_DAC
static const uint32_t               gpioDACBitMask = DAC_BUS_MASK;
#endif

static inline uint32_t getDACMaxRange(void) {
    return (uint32_t)(1 << DAC_SAMPLE_BIT_DEPTH);
}

static bool waveTypeIsEqual(waveform_type_t * t) {
    if (t->type == currentWT.type && t->frequency == currentWT.frequency) {
        return true;
    }

    return false;
}

static inline void setCurrentWaveType(waveform_type_t * t) {
    memcpy(&currentWT, t, sizeof(waveform_type_t));
}

static inline void squareWaveHigh(void) {
    gpio_put(SQUARE_WAVE_OUT_PIN, 1);
}

static inline void squareWaveLow(void) {
    gpio_put(SQUARE_WAVE_OUT_PIN, 0);
}

static void toggleSquareWave(void) {
	gpio_xor_mask((1 << SQUARE_WAVE_OUT_PIN));
    // static bool state = false;

	// if (!state) {
    //     squareWaveHigh();
	// 	state = true;
	// }
	// else {
    //     squareWaveLow();
	// 	state = false;
	// }
}

static uint32_t generateTriangleWave(uint32_t frequency) {
    double          gradient;
    uint32_t        sampleNum;
    uint32_t        countDown;
    uint32_t        cycle_us;
    uint32_t        sampleDelay_us = 0;

    cycle_us = 1000000 / frequency;
    numSamplesPerCycle = cycle_us / MIN_DAC_SAMPLE_DELAY_US;

    if (numSamplesPerCycle > DAC_SAMPLE_RATE) {
        numSamplesPerCycle = DAC_SAMPLE_RATE;
    }

    gradient = (double)(getDACMaxRange() - 1) / (double)(numSamplesPerCycle >> 1);

    sampleDelay_us = cycle_us / numSamplesPerCycle;

    for (sampleNum = 0;sampleNum < (numSamplesPerCycle >> 1);sampleNum++) {
        _samples[sampleNum] = (uint16_t)(gradient * (double)sampleNum);
    }

    /*
    ** Downslope is the opposite of the upslope...
    */
    countDown = sampleNum - 1;

    for (sampleNum = (numSamplesPerCycle >> 1);sampleNum < numSamplesPerCycle;sampleNum++) {
        _samples[sampleNum] = _samples[countDown--];
    }
    
    return sampleDelay_us;
}

static uint32_t generateSawtoothWave(uint32_t frequency) {
    double          gradient;
    uint32_t        sampleNum;
    uint32_t        reverseSampleNum;
    uint32_t        cycle_us;
    uint32_t        sampleDelay_us = 0;

    cycle_us = 1000000 / frequency;
    numSamplesPerCycle = cycle_us / MIN_DAC_SAMPLE_DELAY_US;

    if (numSamplesPerCycle > DAC_SAMPLE_RATE) {
        numSamplesPerCycle = DAC_SAMPLE_RATE;
    }

    gradient = (double)(getDACMaxRange() - 1) / (double)numSamplesPerCycle;

    sampleDelay_us = cycle_us / numSamplesPerCycle;

    reverseSampleNum = numSamplesPerCycle;

    for (sampleNum = 0;sampleNum < numSamplesPerCycle;sampleNum++) {
        _samples[sampleNum] = (uint16_t)(gradient * (double)reverseSampleNum--);
    }
    
    return sampleDelay_us;
}

static uint32_t generateSineWave(uint32_t frequency) {
    double          sampleIntervalDegrees;
    double          angle = 0.0;
    uint32_t        sampleNum;
    uint32_t        cycle_us;
    uint32_t        sampleDelay_us = 0;

    cycle_us = 1000000 / frequency;
    numSamplesPerCycle = cycle_us / MIN_DAC_SAMPLE_DELAY_US;

    if (numSamplesPerCycle > DAC_SAMPLE_RATE) {
        numSamplesPerCycle = DAC_SAMPLE_RATE;
    }

    sampleDelay_us = cycle_us / numSamplesPerCycle;

    sampleIntervalDegrees = ((double)360.0 / (double)numSamplesPerCycle);

    for (sampleNum = 0;sampleNum < numSamplesPerCycle;sampleNum++) {
        _samples[sampleNum] = (uint16_t)((sin(radians(angle)) + 1.0) * (double)(getDACMaxRange() >> 1));

        if (_samples[sampleNum] > getDACMaxRange() - 1) {
            _samples[sampleNum] = getDACMaxRange() - 1;
        }

        angle += sampleIntervalDegrees;

        if (angle > 360.0) {
            angle = angle - 360.0;
        }
    }

    return sampleDelay_us;
}

static void pushDACSample(uint16_t sample, bool isFirst) {
#ifndef USE_MCP4725_DAC
    gpio_put_masked(gpioDACBitMask, ((uint32_t)sample << DAC_PIN_D0));
#else
    writeSampleFast(i2c0, sample, isFirst, false);
#endif
}

static void setupDACPins(void) {
#ifndef USE_MCP4725_DAC
    uint                i;

    for (i = DAC_PIN_D0;i <= DAC_PIN_D13;i++) {
        gpio_set_function(i, GPIO_FUNC_SIO);
        gpio_set_dir(i, GPIO_OUT);
        gpio_set_drive_strength(i, GPIO_DRIVE_STRENGTH_2MA);
        gpio_set_slew_rate(i, GPIO_SLEW_RATE_FAST);
    }
#else
    i2c_init(i2c0, 400000);

    gpio_set_function(I2C0_SDA_ALT_PIN, GPIO_FUNC_I2C);
    gpio_set_function(I2C0_SLK_ALT_PIN, GPIO_FUNC_I2C);
#endif
}

void wave_isr(void) {
	uint32_t			data = 0;
    waveform_type_t     wt;

    while (multicore_fifo_rvalid()) {
        data = multicore_fifo_pop_blocking();
    
        wt.type = (uint16_t)(data & 0x0000FFFF);
        wt.frequency = (uint16_t)((data & 0xFFFF0000) >> 16);

        if (!waveTypeIsEqual(&wt)) {
            setCurrentWaveType(&wt);
            doUpdate = true;
            sampleNum = 0;
        }
    }

    multicore_fifo_clear_irq();
}

void wave_entry(void) {
    uint64_t        delayus = 0;
    bool            isFirst = true;

    currentWT.type = WAVEFORM_TYPE_OFF;
    currentWT.frequency = 0;

    memset(&_samples, 0, sizeof(_samples));

    gpio_set_function(SQUARE_WAVE_OUT_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(SQUARE_WAVE_OUT_PIN, true);
    gpio_put(SQUARE_WAVE_OUT_PIN, false);

    gpio_set_function(CORE1_DEBUG_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(CORE1_DEBUG_PIN, true);
    gpio_put(CORE1_DEBUG_PIN, false);

    setupDACPins();

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, wave_isr);

    irq_set_enabled(SIO_IRQ_PROC1, true);

	while (1) {
        if (doUpdate) {
            doUpdate = false;

            sampleNum = 0;

            gpio_put(CORE1_DEBUG_PIN, true);
            rtcDelay(1000);
            gpio_put(CORE1_DEBUG_PIN, false);

            writeSampleFast(i2c0, 0x0000, isFirst, true);

            switch (currentWT.type) {
                case WAVEFORM_TYPE_SQUARE:
                    delayus = (uint64_t)((double)500000.0 / (double)currentWT.frequency);
                    break;

                case WAVEFORM_TYPE_SINE:
                    squareWaveLow();

                    delayus = (uint64_t)generateSineWave(currentWT.frequency);

                    isFirst = true;
                    break;

                case WAVEFORM_TYPE_TRIANGLE:
                    squareWaveLow();

                    delayus = generateTriangleWave(currentWT.frequency);

                    isFirst = true;
                    break;

                case WAVEFORM_TYPE_SAWTOOTH:
                    squareWaveLow();

                    delayus = generateSawtoothWave(currentWT.frequency);

                    isFirst = true;
                    break;

                default:
                    squareWaveLow();

                    /*
                    ** Switch off the DAC output...
                    */
                    pushDACSample(0x0000, isFirst);
                    break;
            }
        }

        rtcDelay(delayus);
        
        switch (currentWT.type) {
            case WAVEFORM_TYPE_SQUARE:
                toggleSquareWave();
                break;

            case WAVEFORM_TYPE_TRIANGLE:
            case WAVEFORM_TYPE_SAWTOOTH:
            case WAVEFORM_TYPE_SINE:
                /*
                ** Push the DAC sample to the DAC...
                */
                pushDACSample(_samples[sampleNum++], isFirst);
                
                if (isFirst) {
                    isFirst = false;
                }

                if (sampleNum == numSamplesPerCycle) {
                    sampleNum = 0;
                }
                break;

            default:
                squareWaveLow();
                pushDACSample(0x0000, isFirst);
                sampleNum = 0;

                __wfi();
                break;
        }
	}
}
