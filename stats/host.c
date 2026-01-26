#include "host.h"
#include <unistd.h>

int get_hostname(char *buf, int buflen) {
    if (gethostname(buf, buflen) != 0)
        return -1;

    buf[buflen - 1] = '\0';
    return 0;
}
