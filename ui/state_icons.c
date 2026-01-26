#include "state_icons.h"
#include "../fb/bitmap_loader.h"

#ifndef ICON_PATH
#define ICON_PATH "/usr/local/share/oled-status/icons/"
#endif
#define ICON_W 16
#define ICON_H 16

int load_state_icons(struct state_icons *icons) {
    if (load_bitmap(ICON_PATH "status_booting_16x16.bin", ICON_W, ICON_H, &icons->booting)) return -1;
    if (load_bitmap(ICON_PATH "status_ok_16x16.bin", ICON_W, ICON_H, &icons->booted)) return -1;
    if (load_bitmap(ICON_PATH "status_degraded_16x16.bin", ICON_W, ICON_H, &icons->degraded)) return -1;
    if (load_bitmap(ICON_PATH "status_failed_16x16.bin", ICON_W, ICON_H, &icons->failed)) return -1;
    return 0;
}

void free_state_icons(struct state_icons *icons) {
    free_bitmap(&icons->booting);
    free_bitmap(&icons->booted);
    free_bitmap(&icons->degraded);
    free_bitmap(&icons->failed);
}
