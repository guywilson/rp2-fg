#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "scheduler.h"
#include "schederr.h"

#include "taskdef.h"

#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "pico/multicore.h"
#include "hardware/watchdog.h"
#include "hardware/i2c.h"
#include "hardware/gpio.h"
#include "rtc_rp2040.h"
#include "serial_rp2040.h"
#include "heartbeat.h"
#include "watchdog.h"
#include "logger.h"
#include "utils.h"
#include "gpio_def.h"
#include "waveform.h"
#include "MCP4725.h"
#include "AiP31068.h"

#define MAX_WT_POOL_SIZE                5

void taskDebugCheck(PTASKPARM p) {
    // if (isDebugActive()) {
    //     lgSetLogLevel(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_STATUS | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO);
    // }
    // else {
    //     lgSetLogLevel(LOG_LEVEL_OFF);
    // }
}

void taskWaveDebug(PTASKPARM p) {
    static uint16_t         state = WAVEFORM_TYPE_SAWTOOTH;
    waveform_type_t         wt;
    uint32_t                data;
    const uint16_t          frequency = 2000;

    switch (state) {
        case WAVEFORM_TYPE_OFF:
            wt.type = WAVEFORM_TYPE_OFF;
            wt.frequency = 0;
            state = WAVEFORM_TYPE_SAWTOOTH;
            break;

        case WAVEFORM_TYPE_SAWTOOTH:
            wt.type = WAVEFORM_TYPE_SAWTOOTH;
            wt.frequency = frequency;
            state = WAVEFORM_TYPE_TRIANGLE;
            break;

        case WAVEFORM_TYPE_TRIANGLE:
            wt.type = WAVEFORM_TYPE_TRIANGLE;
            wt.frequency = frequency;
            state = WAVEFORM_TYPE_SINE;
            break;

        case WAVEFORM_TYPE_SINE:
            wt.type = WAVEFORM_TYPE_SINE;
            wt.frequency = frequency;
            state = WAVEFORM_TYPE_OFF;
            break;

        // case WAVEFORM_TYPE_SQUARE:
        //     wt.frequency = frequency;
        //     break;
    }

    data = ((uint32_t)wt.frequency << 16) | (uint32_t)wt.type;

    lgLogDebug("Pushing WT, T:0x%04X, F:%U", wt.type, wt.frequency);
    if (multicore_fifo_push_timeout_us(data, 25)) {
        lgLogDebug("Pushed WT");
    }
    else {
        lgLogError("Timeout pushing data to FIFO");
    }
}

static void setup(void) {
	/*
	** Disable the Watchdog, if we have restarted due to a
	** watchdog reset, we want to enable it when we're ready...
	*/
	watchdog_disable();

	setupLEDPin();
    setupDebugPin();
	setupRTC();

    rtcDelay(1000000U);
}

int main(void) {
	setup();

	multicore_reset_core1();
	multicore_fifo_drain();

	/*
	** Launch the wave generator on the 2nd core...
	*/
	multicore_launch_core1(wave_entry);

	initScheduler(6);

	registerTask(TASK_HEARTBEAT, &HeartbeatTask);
	registerTask(TASK_WATCHDOG, &taskWatchdog);
    registerTask(TASK_WAVE_DEBUG, &taskWaveDebug);
    registerTask(TASK_LCD_INITIALISE, &taskLCDInitialise);
    registerTask(TASK_LCD_PRINT, &taskLCDPrint);
    registerTask(TASK_DEBUG_CHECK, &taskDebugCheck);

	scheduleTask(
			TASK_HEARTBEAT,
			rtc_val_sec(1),
            false,
			NULL);

	scheduleTask(
			TASK_WATCHDOG, 
			rtc_val_ms(50), 
            true, 
			NULL);

	scheduleTask(
			TASK_WAVE_DEBUG, 
			rtc_val_sec(10), 
            true, 
			NULL);

	// scheduleTask(
	// 		TASK_DEBUG_CHECK, 
	// 		rtc_val_sec(1), 
    //         true, 
	// 		NULL);

    lcdSetup_AIP31068(i2c0);

	/*
	** Enable the watchdog, it will reset the device in 100ms unless
	** the watchdog timer is updated by WatchdogTask()...
	*/
	watchdog_enable(100, false);

	/*
	** Start the scheduler...
	*/
	schedule();

	return -1;
}
