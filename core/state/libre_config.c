#include "libre_config.h"
#include <stdio.h>
#include <string.h>

static char url[256] = "";
static char token[128] = "";

void libre_config_load(void) {
    /* Try local config first (for development/simulator) */
    FILE *f = fopen("config/oled-statusd.conf", "r");
    
    /* If not found, try system location (for production) */
    if (!f) {
        f = fopen("/etc/oled-statusd.conf", "r");
    }

    if (!f)
        return;

    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "LIBRE_URL=", 10) == 0)
            strncpy(url, line + 10, sizeof(url));
        else if (strncmp(line, "LIBRE_TOKEN=", 12) == 0)
            strncpy(token, line + 12, sizeof(token));
    }

    fclose(f);

    /* Strip newline */
    url[strcspn(url, "\n")] = 0;
    token[strcspn(token, "\n")] = 0;
}

const char *libre_url(void) { return url; }
const char *libre_token(void) { return token; }
