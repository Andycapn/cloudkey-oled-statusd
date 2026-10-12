#pragma once
#include <stdbool.h>
#include <stdint.h>
#include "fb.h"

void draw_char(struct fb *fb, int x, int y, char c, uint16_t color);
void draw_text(struct fb *fb, int x, int y, const char *s, uint16_t color);
void draw_text_large(struct fb *fb, int x, int y, const char *s, uint16_t color);
void draw_text_fit(struct fb *fb, int x, int y, const char *s, int max_width,
				   bool large, uint16_t color);
