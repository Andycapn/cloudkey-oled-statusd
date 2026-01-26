#include "system.h"
#include <stdio.h>

int read_uptime_seconds(long *out) {
    FILE *f = fopen("/proc/uptime", "r");
    if (!f)
        return -1;

    double up = 0;
    if (fscanf(f, "%lf", &up) != 1) {
        fclose(f);
        return -1;
    }

    fclose(f);
    *out = (long)up;
    return 0;
}
