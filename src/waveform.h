#include <stdint.h>

#ifndef _INCL_WAVEFORM
#define _INCL_WAVEFORM

#define WAVEFORM_TYPE_OFF               0xFFFF
#define WAVEFORM_TYPE_SQUARE            0x0001
#define WAVEFORM_TYPE_SINE              0x0002
#define WAVEFORM_TYPE_SAWTOOTH          0x0004
#define WAVEFORM_TYPE_TRIANGLE          0x0008

#define DAC_SAMPLE_RATE             62500
#define DAC_SAMPLE_BIT_DEPTH        12

#define MAX_SUPPORTED_FREQUENCY     20000
#define MIN_DAC_SAMPLE_DELAY_US     2

typedef struct {
    uint16_t    type;
    uint16_t    frequency;
}
waveform_type_t;

void wave_isr(void);
void wave_entry(void);

#endif
