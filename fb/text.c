#include "text.h"
#include "draw.h"
#include "../assets/font_jetbrains_mono_8.h"

static void draw_glyph(struct fb *fb, int x, int y, const uint8_t *glyph, uint16_t color) {
    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 8; row++) {
            if (bits & (1 << row)) {
                draw_pixel(fb, x + col, y + row, color);
            }
        }
    }
}

void draw_char(struct fb *fb, int x, int y, char c, uint16_t color) {
    if (c >= 32 && c <= 126) {
        draw_glyph(fb, x, y, font_jb_mono[c - 32], color);
    }
}

void draw_text(struct fb *fb, int x, int y, const char *s, uint16_t color) {
    int cx = x;
    while (*s) {
        if (*s == ' ') {
            cx += 6;
        } else {
            draw_char(fb, cx, y, *s, color);
            cx += 6;
        }
        s++;
    }
}

