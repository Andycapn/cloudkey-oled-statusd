#pragma once
#include <stdint.h>
#include <stdbool.h>

#define FB_PHYS_HEIGHT    60
#define FB_VISIBLE_HEIGHT 58

#define FB_WIDTH  160
#define FB_HEIGHT 60

struct fb_backend;

struct fb {
    uint16_t *buf;
    struct fb_backend *backend;
    uint32_t led_color; // Simulated LED color (RGB888)
};

int  fb_init(struct fb *fb);
void fb_clear(struct fb *fb, uint16_t color);
void fb_present(struct fb *fb);
void fb_close(struct fb *fb);
bool fb_should_close(struct fb *fb);
