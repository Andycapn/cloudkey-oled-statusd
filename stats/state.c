#include "state.h"
#include "network.h"

enum system_state get_system_state(void) {
    long uptime = 0;

    if (read_uptime_seconds(&uptime) != 0)
        return STATE_FAILED;

    if (uptime < 120)
        return STATE_BOOTING;

    if (network_is_up())
        return STATE_BOOTED;

    return STATE_DEGRADED;
}

const char *state_to_string(enum system_state s) {
    switch (s) {
    case STATE_BOOTING:  return "BOOTING";
    case STATE_BOOTED:   return "BOOTED";
    case STATE_DEGRADED: return "DEGRADED";
    case STATE_FAILED:   return "FAILED";
    default:             return "UNKNOWN";
    }
}

