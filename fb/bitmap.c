#include "bitmap.h"
#include "draw.h"

void draw_bitmap(struct fb *fb, int x, int y, const struct bitmap *bmp, uint16_t color) {
    int byte_width = (bmp->w + 7) / 8;

    for (int row = 0; row < bmp->h; row++) {
        for (int col = 0; col < bmp->w; col++) {
            int byte = row * byte_width + (col / 8);
            int bit  = 7 - (col % 8);

            if (bmp->data[byte] & (1 << bit)) {
                draw_pixel(fb, x + col, y + row, color);
            }
        }
    }
}
