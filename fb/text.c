#include "text.h"
#include "draw.h"
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

static const uint8_t *glyph_for_char(char c) {
    if (c == ' ') return font5x7_space;
    if (c == '.') return font5x7_dot;
    if (c == ':') return font5x7_colon;
    if (c == '-') return font5x7_dash;
    if (c == '/') return font5x7_slash;
    if (c >= '0' && c <= '9') return font5x7_digits[c - '0'];
    if (c >= 'A' && c <= 'Z') return font5x7_alpha[c - 'A'];
    if (c >= 'a' && c <= 'z') return font5x7_alpha[c - 'a'];
    return font5x7_space;
}

void draw_char(struct fb *fb, int x, int y, char c, uint16_t color) {
    const uint8_t *glyph = glyph_for_char(c);
    draw_glyph(fb, x, y, glyph, color);
}

void draw_text(struct fb *fb, int x, int y, const char *s, uint16_t color) {
    int cx = x;
    while (*s) {
        draw_char(fb, cx, y, *s, color);
        cx += 6;
        s++;
    }
}

