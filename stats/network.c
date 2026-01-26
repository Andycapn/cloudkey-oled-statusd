#include "network.h"
#include <dirent.h>
#include <stdio.h>
#include <string.h>

int network_is_up(void) {
    DIR *d = opendir("/sys/class/net");
    if (!d)
        return 0;

    struct dirent *de;
    while ((de = readdir(d))) {
        if (de->d_name[0] == '.')
            continue;

        if (strcmp(de->d_name, "lo") == 0)
            continue;

        char path[512];
        snprintf(path, sizeof(path),
                 "/sys/class/net/%s/carrier", de->d_name);

        FILE *f = fopen(path, "r");
        if (!f)
            continue;

        int carrier = 0;
        fscanf(f, "%d", &carrier);
        fclose(f);

        if (carrier == 1) {
            closedir(d);
            return 1;
        }
    }

    closedir(d);
    return 0;
}
