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
#include "rtc_rp2040.h"
#include "serial_rp2040.h"
#include "heartbeat.h"
#include "watchdog.h"
#include "logger.h"
#include "utils.h"
#include "gpio_def.h"
#include "waveform.h"

#define MAX_WT_POOL_SIZE                5

void taskDebugCheck(PTASKPARM p) {
    if (isDebugActive()) {
        lgSetLogLevel(LOG_LEVEL_FATAL | LOG_LEVEL_ERROR | LOG_LEVEL_STATUS | LOG_LEVEL_DEBUG | LOG_LEVEL_INFO);
    }
    else {
        lgSetLogLevel(LOG_LEVEL_OFF);
    }
}

void taskSquareWaveDebug(PTASKPARM p) {
    static bool             isOn = false;
    waveform_type_t         wt;
    uint32_t                data;

    if (isOn) {
        isOn = false;
        wt.type = WAVEFORM_TYPE_OFF;
        wt.frequency = 0;
    }
    else {
        isOn = true;
        wt.type = WAVEFORM_TYPE_SQUARE;
        wt.frequency = 1000;
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
	setupSerial();

    lgOpen(uart0, LOG_LEVEL_OFF);

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

    lgLogDebug("In main()");

	initScheduler(4);

	registerTask(TASK_HEARTBEAT, &HeartbeatTask);
	registerTask(TASK_WATCHDOG, &taskWatchdog);
    registerTask(TASK_SQUAREWAVE_DEBUG, &taskSquareWaveDebug);
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
			TASK_SQUAREWAVE_DEBUG, 
			rtc_val_sec(5), 
            true, 
			NULL);

	// scheduleTask(
	// 		TASK_DEBUG_CHECK, 
	// 		rtc_val_sec(1), 
    //         true, 
	// 		NULL);

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
