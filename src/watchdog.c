#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "scheduler.h"

#include "hardware/watchdog.h"
#include "rtc_rp2040.h"
#include "taskdef.h"
#include "watchdog.h"

static bool         doUpdate = true;

void watchdog_disable(void) {
	hw_clear_bits(&watchdog_hw->ctrl, WATCHDOG_CTRL_ENABLE_BITS);
}

void triggerWatchdogReset(void) {
    doUpdate = false;
}

void taskWatchdog(PTASKPARM p) {
    if (doUpdate) {
        watchdog_update();
    }
}
