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
