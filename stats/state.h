#pragma once
#include "system.h"

enum system_state {
    STATE_BOOTING,
    STATE_BOOTED,
    STATE_DEGRADED,
    STATE_FAILED
};

enum system_state get_system_state(void);
const char *state_to_string(enum system_state s);
