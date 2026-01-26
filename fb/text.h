#pragma once
#include <stdint.h>
#include "fb.h"

void draw_char(struct fb *fb, int x, int y, char c, uint16_t color);
void draw_text(struct fb *fb, int x, int y, const char *s, uint16_t color);
