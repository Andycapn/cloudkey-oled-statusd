#include "libre_config.h"
#include <ctype.h>
#include <stdio.h>
#include <string.h>

static char url[256] = "";
static char token[128] = "";
static char core_device[64] = "";
static char core_ports[256] = "";

static void copy_config_value(char *destination, size_t capacity, const char *value) {
    while (isspace((unsigned char)*value)) {
        value++;
    }

    size_t length = strcspn(value, "\r\n");
    while (length > 0 && isspace((unsigned char)value[length - 1])) {
        length--;
    }
    if (length >= capacity) {
        length = capacity - 1;
    }

    memcpy(destination, value, length);
    destination[length] = '\0';
}

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
            copy_config_value(url, sizeof(url), line + 10);
        else if (strncmp(line, "LIBRE_TOKEN=", 12) == 0)
            copy_config_value(token, sizeof(token), line + 12);
        else if (strncmp(line, "LIBRE_CORE_DEVICE=", 18) == 0)
            copy_config_value(core_device, sizeof(core_device), line + 18);
        else if (strncmp(line, "LIBRE_CORE_PORTS=", 17) == 0)
            copy_config_value(core_ports, sizeof(core_ports), line + 17);
    }

    fclose(f);
}

const char *libre_url(void) { return url; }
const char *libre_token(void) { return token; }
const char *libre_core_device(void) { return core_device; }
const char *libre_core_ports(void) { return core_ports; }
