#pragma once
#include "../fb/fb.h"

enum screen_icon {
    SCREEN_ICON_CLOUD,
    SCREEN_ICON_DEVICE,
    SCREEN_ICON_ALERT,
    SCREEN_ICON_CPU,
    SCREEN_ICON_MEMORY,
    SCREEN_ICON_DISK,
    SCREEN_ICON_CLOCK,
    SCREEN_ICON_NETWORK,
    SCREEN_ICON_DOWNLOAD,
    SCREEN_ICON_UPLOAD,
    SCREEN_ICON_LATENCY,
    SCREEN_ICON_LOSS,
};

struct screen_metric {
    enum screen_icon icon;
    const char *label;
    const char *value;
};

void screen_chrome_begin(struct fb *fb, const char *title);
void screen_chrome_divider(struct fb *fb);
void screen_chrome_footer(struct fb *fb, const struct screen_metric metrics[3]);