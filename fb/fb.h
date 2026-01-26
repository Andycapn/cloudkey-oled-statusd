#pragma once
#include <stdint.h>

#define FB_PHYS_HEIGHT    60
#define FB_VISIBLE_HEIGHT 58

#define FB_WIDTH  160
#define FB_HEIGHT 60

struct fb {
    int fd;
    uint16_t *buf;
};

int  fb_init(struct fb *fb);
void fb_clear(struct fb *fb, uint16_t color);
void fb_present(struct fb *fb);
void fb_close(struct fb *fb);
