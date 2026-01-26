#pragma once
#include <stdint.h>
#include "fb.h"

void draw_pixel(struct fb *fb, int x, int y, uint16_t color);
void draw_rect(struct fb *fb, int x, int y, int w, int h, uint16_t color);
void draw_hline(struct fb *fb, int x, int y, int w, uint16_t color);
void draw_vline(struct fb *fb, int x, int y, int h, uint16_t color);
