// -------------------------------------------------- //
// This file is autogenerated by pioasm; do not edit! //
// -------------------------------------------------- //

#pragma once

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

// ---- //
// wave //
// ---- //

#define wave_wrap_target 0
#define wave_wrap 0

static const uint16_t wave_program_instructions[] = {
            //     .wrap_target
    0x600e, //  0: out    pins, 14                   
            //     .wrap
};

#if !PICO_NO_HARDWARE
static const struct pio_program wave_program = {
    .instructions = wave_program_instructions,
    .length = 1,
    .origin = -1,
};

static inline pio_sm_config wave_program_get_default_config(uint offset) {
    pio_sm_config c = pio_get_default_sm_config();
    sm_config_set_wrap(&c, offset + wave_wrap_target, offset + wave_wrap);
    return c;
}
#endif

