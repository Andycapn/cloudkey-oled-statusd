#include <stdio.h>
#include "stats/state.h"

int main(void) {
    enum system_state s = get_system_state();
    printf("STATE: %s\n", state_to_string(s));
    return 0;
}
