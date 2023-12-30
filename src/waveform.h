#include <stdint.h>

#ifndef _INCL_WAVEFORM
#define _INCL_WAVEFORM

#define WAVEFORM_TYPE_SQUARE          0x0001
#define WAVEFORM_TYPE_SINE            0x0002
#define WAVEFORM_TYPE_SAWTOOTH        0x0004
#define WAVEFORM_TYPE_TRIANGLE        0x0008

typedef struct {
    int         type;

    uint32_t    frequency;
}
waveform_type_t;

void generateSquareWave(uint32_t frequency);
void generateTriangleWave(uint32_t frequency);
void generateSawtoothWave(uint32_t frequency);
void generateSineWave(uint32_t frequency);

#endif
