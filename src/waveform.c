#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"

#include "logger.h"
#include "rtc_rp2040.h"
#include "gpio_def.h"
#include "waveform.h"

#define PI                          3.1415926535
#define DEGREE_TO_RADIANS           (PI / 180.0)

#define radians(d)                  (d * DEGREE_TO_RADIANS)

static waveform_type_t              currentWT;
static volatile bool                doUpdate = false;

static uint32_t                     numSamplesPerCycle = 0;

static uint16_t _samples[DAC_SAMPLE_RATE];

static uint32_t getDACMaxRange(void) {
    static uint32_t         maxRange = 0;

    if (maxRange == 0) {
        maxRange = (2 ^ DAC_SAMPLE_BIT_DEPTH);
    }

    return maxRange;
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
	static unsigned char state = 0;
	
	if (!state) {
        squareWaveHigh();
		state = 1;
	}
	else {
        squareWaveLow();
		state = 0;
	}
}

static void pushDACSample(uint16_t sample) {
    gpio_put_masked(DAC_BUS_MASK, sample);
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

        angle += sampleIntervalDegrees;

        if (angle > 360.0) {
            angle = angle - 360.0;
        }
    }

    return sampleDelay_us;
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
        }
    }

    multicore_fifo_clear_irq();
}

void wave_entry(void) {
    uint64_t        delayus = 0;
    int             sampleNum = 0;

    currentWT.type = WAVEFORM_TYPE_OFF;
    currentWT.frequency = 0;

    memset(&_samples, 0, sizeof(_samples));

    gpio_set_function(SQUARE_WAVE_OUT_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(SQUARE_WAVE_OUT_PIN, true);
    gpio_put(SQUARE_WAVE_OUT_PIN, false);

    gpio_set_function(CORE1_DEBUG_PIN, GPIO_FUNC_SIO);
    gpio_set_dir(CORE1_DEBUG_PIN, true);
    gpio_put(CORE1_DEBUG_PIN, false);

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, wave_isr);

    irq_set_enabled(SIO_IRQ_PROC1, true);

	while (1) {
        if (doUpdate) {
            doUpdate = false;

            gpio_put(CORE1_DEBUG_PIN, true);
            rtcDelay(1000);
            gpio_put(CORE1_DEBUG_PIN, false);

            switch (currentWT.type) {
                case WAVEFORM_TYPE_SQUARE:
                    delayus = (uint64_t)((double)500000.0 / (double)currentWT.frequency);
                    break;

                case WAVEFORM_TYPE_SINE:
                    squareWaveLow();

                    delayus = (uint64_t)generateSineWave(currentWT.frequency);
                    break;

                case WAVEFORM_TYPE_TRIANGLE:
                    squareWaveLow();
                    delayus = (uint64_t)((double)1 / (double)DAC_SAMPLE_RATE * 1000000);

                    generateTriangleWave(currentWT.frequency);
                    break;

                case WAVEFORM_TYPE_SAWTOOTH:
                    squareWaveLow();
                    delayus = (uint64_t)((double)1 / (double)DAC_SAMPLE_RATE * 1000000);

                    generateSawtoothWave(currentWT.frequency);
                    break;

                default:
                    squareWaveLow();

                    /*
                    ** Switch off the DAC output...
                    */
                    pushDACSample(0x0000);
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
                pushDACSample(_samples[sampleNum++]);
                
                if (sampleNum == numSamplesPerCycle) {
                    sampleNum = 0;
                }
                break;

            default:
                squareWaveLow();
                pushDACSample(0x0000);

                __wfi();
                break;
        }
	}
}
