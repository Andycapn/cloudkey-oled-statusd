#include "draw.h"

static inline int in_bounds(int x, int y) {
    return x >= 0 &&
           x < FB_WIDTH &&
           y >= 0 &&
           y < FB_VISIBLE_HEIGHT;
}

void draw_pixel(struct fb *fb, int x, int y, uint16_t color) {
    if (!in_bounds(x, y)) return;
    fb->buf[y * FB_WIDTH + x] = color;
}

void draw_pixel_coverage(struct fb *fb, int x, int y, uint16_t color, uint8_t coverage) {
    if (coverage == 0 || !in_bounds(x, y)) return;
    if (coverage >= 15) {
        draw_pixel(fb, x, y, color);
        return;
    }

    int index = y * FB_WIDTH + x;
    uint16_t background = fb->buf[index];
    unsigned int inverse = 15 - coverage;
    unsigned int red = ((((color >> 11) & 0x1F) * coverage) +
                        (((background >> 11) & 0x1F) * inverse) + 7) / 15;
    unsigned int green = ((((color >> 5) & 0x3F) * coverage) +
                          (((background >> 5) & 0x3F) * inverse) + 7) / 15;
    unsigned int blue = (((color & 0x1F) * coverage) +
                         ((background & 0x1F) * inverse) + 7) / 15;
    fb->buf[index] = (uint16_t)((red << 11) | (green << 5) | blue);
}

void draw_rect(struct fb *fb, int x, int y, int w, int h, uint16_t color) {
    for (int yy = y; yy < y + h; yy++) {
        for (int xx = x; xx < x + w; xx++) {
            draw_pixel(fb, xx, yy, color);
        }
    }
}

void draw_hline(struct fb *fb, int x, int y, int w, uint16_t color) {
    for (int i = 0; i < w; i++) {
        draw_pixel(fb, x + i, y, color);
    }
}

void draw_vline(struct fb *fb, int x, int y, int h, uint16_t color) {
    for (int i = 0; i < h; i++) {
        draw_pixel(fb, x, y + i, color);
    }
}
