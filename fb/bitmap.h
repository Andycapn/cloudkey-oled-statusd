#pragma once
#include <stdint.h>
#include "fb.h"

struct bitmap {
    int w;
    int h;
    const uint8_t *data;
};

void draw_bitmap(struct fb *fb, int x, int y, const struct bitmap *bmp, uint16_t color);
