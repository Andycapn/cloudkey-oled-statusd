#include "text.h"
#include "draw.h"
#include "../assets/font_opensans.h"
#include <stddef.h>
#include <string.h>

static const struct bitmap_glyph *glyph_for_char(char c) {
    if (c < ' ' || c > '~') {
        c = '?';
    }
    return &opensans_small[(unsigned char)c - ' '];
}

static void draw_glyph(struct fb *fb, int x, int y, const struct bitmap_glyph *glyph,
                       int height, uint16_t color) {
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < glyph->width; col++) {
            unsigned int coverage = (unsigned int)((glyph->rows[row] >> (col * 4)) & 0x0F);
            int pixel_x = x + glyph->x_offset + col;
            int pixel_y = y + row;
            if (coverage == 0 || pixel_x < 0 || pixel_x >= FB_WIDTH ||
                pixel_y < 0 || pixel_y >= FB_VISIBLE_HEIGHT) {
                continue;
            }
            draw_pixel_coverage(fb, pixel_x, pixel_y, color, (uint8_t)coverage);
        }
    }
}

static char printable_char(char c) {
    return c < ' ' || c > '~' ? '?' : c;
}

static int measure_text(const char *s, size_t length, const struct bitmap_glyph *font) {
    int width = 0;
    for (size_t i = 0; i < length; i++) {
        char c = printable_char(s[i]);
        width += font[(unsigned char)c - ' '].advance;
    }
    return width;
}

static void draw_text_with_font_n(struct fb *fb, int x, int y, const char *s, size_t length,
                                  const struct bitmap_glyph *font, int height,
                                  uint16_t color) {
    int pen_x = x;
    for (size_t i = 0; i < length; i++) {
        char c = printable_char(s[i]);
        const struct bitmap_glyph *glyph = &font[(unsigned char)c - ' '];
        draw_glyph(fb, pen_x, y, glyph, height, color);
        pen_x += glyph->advance;
    }
}

static void draw_text_with_font(struct fb *fb, int x, int y, const char *s,
                                const struct bitmap_glyph *font, int height,
                                uint16_t color) {
    draw_text_with_font_n(fb, x, y, s, strlen(s), font, height, color);
}

void draw_char(struct fb *fb, int x, int y, char c, uint16_t color) {
    draw_glyph(fb, x, y, glyph_for_char(c), OPEN_SANS_SMALL_HEIGHT, color);
}

void draw_text(struct fb *fb, int x, int y, const char *s, uint16_t color) {
    draw_text_with_font(fb, x, y, s, opensans_small, OPEN_SANS_SMALL_HEIGHT, color);
}

void draw_text_large(struct fb *fb, int x, int y, const char *s, uint16_t color) {
    draw_text_with_font(fb, x, y, s, opensans_large, OPEN_SANS_LARGE_HEIGHT, color);
}

void draw_text_fit(struct fb *fb, int x, int y, const char *s, int max_width,
                   bool large, uint16_t color) {
    const struct bitmap_glyph *font = large ? opensans_large : opensans_small;
    int height = large ? OPEN_SANS_LARGE_HEIGHT : OPEN_SANS_SMALL_HEIGHT;
    size_t length = strlen(s);

    if (max_width <= 0) {
        return;
    }
    if (measure_text(s, length, font) <= max_width) {
        draw_text_with_font_n(fb, x, y, s, length, font, height, color);
        return;
    }

    static const char ellipsis[] = "...";
    int ellipsis_width = measure_text(ellipsis, sizeof(ellipsis) - 1, font);
    while (length > 0 && measure_text(s, length, font) + ellipsis_width > max_width) {
        length--;
    }
    if (ellipsis_width > max_width) {
        return;
    }

    draw_text_with_font_n(fb, x, y, s, length, font, height, color);
    draw_text_with_font(fb, x + measure_text(s, length, font), y, ellipsis,
                        font, height, color);
}

