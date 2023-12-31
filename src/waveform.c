#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>

#include "pico/multicore.h"
#include "gpio_def.h"
#include "waveform.h"

#define DAC_SAMPLE_RATE         192000
#define DAC_SAMPLE_BIT_DEPTH        12

static uint32_t getDACMaxRange(void) {
    static uint32_t         maxRange = 0;

    if (maxRange == 0) {
        maxRange = (2 ^ DAC_SAMPLE_BIT_DEPTH);
    }

    return maxRange;
}

static void triggerSquareWave(uint32_t frequency) {

}

static void triggerTriangleWave(uint32_t frequency) {

}

static void triggerSawtoothWave(uint32_t frequency) {

}

static void triggerSineWave(uint32_t frequency) {

}

void wave_isr(void) {
	uint32_t			data = 0;
    waveform_type_t *   wt;

    while (multicore_fifo_rvalid()) {
        data = multicore_fifo_pop_blocking();
    
        wt = (waveform_type_t *)data;

        switch (wt->type) {
            case WAVEFORM_TYPE_SQUARE:
                triggerSquareWave(wt->frequency);
                break;

            case WAVEFORM_TYPE_SINE:
                triggerSineWave(wt->frequency);
                break;

            case WAVEFORM_TYPE_TRIANGLE:
                triggerTriangleWave(wt->frequency);
                break;

            case WAVEFORM_TYPE_SAWTOOTH:
                triggerSawtoothWave(wt->frequency);
                break;
        }
    }

    multicore_fifo_clear_irq();
}

void wave_entry(void) {
    multicore_fifo_clear_irq();
    irq_set_exclusive_handler(SIO_IRQ_PROC1, wave_isr);

    irq_set_enabled(SIO_IRQ_PROC1, true);

	while (1) {
        __wfi();
	}
}
