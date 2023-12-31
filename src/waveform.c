#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "pico/multicore.h"
#include "hardware/pwm.h"
#include "hardware/rtc.h"

#include "rtc_rp2040.h"
#include "gpio_def.h"
#include "waveform.h"

#define DAC_SAMPLE_RATE          62500
#define DAC_SAMPLE_BIT_DEPTH        12

static volatile waveform_type_t     currentWT;
static volatile bool                doUpdate = false;

static int                          numSamplesPerCycle = 0;

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

static void triggerSquareWave(uint slice, uint32_t frequency) {
    /*
    ** Create a PWM square wave... 
    */
    pwm_set_clkdiv(slice, 256.0);
    pwm_set_wrap(slice, 610);
    pwm_set_chan_level(slice, PWM_CHAN_A, 610);
    
    pwm_set_enabled(slice, true);
}

static void generateTriangleWave(uint32_t frequency) {

}

static void generateSawtoothWave(uint32_t frequency) {

}

static void generateSineWave(uint32_t frequency) {

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
                    delayus = (uint64_t)((double)1 / (double)DAC_SAMPLE_RATE * 1000000);

                    generateSineWave(currentWT.frequency);
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

                    gpio_put_masked(
                        DAC_BUS_MASK,
                        (uint32_t)0x00000000);
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
                gpio_put_masked(DAC_BUS_MASK, _samples[sampleNum++]);
                
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
