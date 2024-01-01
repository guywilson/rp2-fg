#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <math.h>

#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"

#include "rtc_rp2040.h"
#include "gpio_def.h"
#include "waveform.h"

#define PI                          3.1415926535
#define DEGREE_TO_RADIANS           (PI / 180.0)

#define radians(d)                  (d * DEGREE_TO_RADIANS)

static volatile waveform_type_t     currentWT;
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
    currentWT.type = t->type;
    currentWT.frequency = t->frequency;
    doUpdate = true;
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
    uint16_t        sampleValue = 0;
    uint16_t        sampleInterval = 0;
    uint32_t        sampleNum;
    uint32_t        cycle_us;
    uint32_t        sampleDelay_us = 0;

    cycle_us = 1000000 / frequency;
    numSamplesPerCycle = cycle_us / MIN_DAC_SAMPLE_DELAY_US;

    if (numSamplesPerCycle > DAC_SAMPLE_RATE) {
        numSamplesPerCycle = DAC_SAMPLE_RATE;
    }

    sampleDelay_us = cycle_us / numSamplesPerCycle;

    sampleInterval = getDACMaxRange() / (numSamplesPerCycle >> 1);

    for (sampleNum = 0;sampleNum < numSamplesPerCycle;sampleNum++) {
        _samples[sampleNum] = sampleValue;

        if (sampleValue <= (getDACMaxRange() - 1)) {
            sampleValue += sampleInterval;
        }
        else {
            sampleValue -= sampleInterval;
        }
    }

    return sampleDelay_us;
}

static uint32_t generateSawtoothWave(uint32_t frequency) {
    uint16_t        sampleValue = getDACMaxRange() - 1;
    uint16_t        sampleInterval = 0;
    uint32_t        sampleNum;
    uint32_t        cycle_us;
    uint32_t        sampleDelay_us = 0;

    cycle_us = 1000000 / frequency;
    numSamplesPerCycle = cycle_us / MIN_DAC_SAMPLE_DELAY_US;

    if (numSamplesPerCycle > DAC_SAMPLE_RATE) {
        numSamplesPerCycle = DAC_SAMPLE_RATE;
    }

    sampleDelay_us = cycle_us / numSamplesPerCycle;

    sampleInterval = getDACMaxRange() / numSamplesPerCycle;

    for (sampleNum = 0;sampleNum < numSamplesPerCycle;sampleNum++) {
        _samples[sampleNum] = sampleValue;

        sampleValue -= sampleInterval;
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
    waveform_type_t *   wt;

    while (multicore_fifo_rvalid()) {
        data = multicore_fifo_pop_blocking();
    
        wt = (waveform_type_t *)data;

        if (!waveTypeIsEqual(wt)) {
            setCurrentWaveType(wt);
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

    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, wave_isr);

    irq_set_enabled(SIO_IRQ_PROC1, true);

	while (1) {
        if (doUpdate) {
            doUpdate = false;

            switch (currentWT.type) {
                case WAVEFORM_TYPE_SQUARE:
                    delayus = (uint64_t)(((double)1 / (double)currentWT.frequency) / (double)2 * 1000000);
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
                __wfi();
                break;
        }
	}
}
