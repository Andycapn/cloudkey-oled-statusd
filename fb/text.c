#include "text.h"
#include "draw.h"
#include "../assets/font5x7.h"
#include "../assets/font5x7_alpha.h"

static void draw_glyph(struct fb *fb, int x, int y, const uint8_t *glyph, uint16_t color) {
    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1 << row)) {
                draw_pixel(fb, x + col, y + row, color);
            }
        }
    }
}

void draw_char(struct fb *fb, int x, int y, char c, uint16_t color) {
    if (c >= '0' && c <= '9') {
        draw_glyph(fb, x, y, font5x7_digits[c - '0'], color);
    } else if (c >= 'A' && c <= 'Z') {
        draw_glyph(fb, x, y, font5x7_alpha[c - 'A'], color);
    } else if (c == '.') {
        draw_glyph(fb, x, y, font5x7_dot, color);
    } else if (c == ':') {
        draw_glyph(fb, x, y, font5x7_colon, color);
    } else if (c == '-') {
        draw_glyph(fb, x, y, font5x7_dash, color);
    } else if (c == '/') {
        draw_glyph(fb, x, y, font5x7_slash, color);
    } else if (c == ' ') {
        /* advance only */
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

