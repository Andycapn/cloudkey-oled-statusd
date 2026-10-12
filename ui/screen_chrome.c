#include "screen_chrome.h"
#include "../fb/draw.h"
#include "../fb/text.h"
#include <math.h>

#define BLACK 0x0000
#define WHITE 0xFFFF
#define MUTED 0x7BEF
#define DIVIDER 0x4208

static const uint8_t icon_rows[][7] = {
    { 0x18, 0x24, 0x42, 0x81, 0x81, 0x7E, 0x00 },
    { 0x7E, 0x42, 0x5A, 0x5A, 0x42, 0x7E, 0x00 },
    { 0x18, 0x24, 0x24, 0x42, 0x42, 0x7E, 0x18 },
    { 0x3C, 0x24, 0x5A, 0x5A, 0x24, 0x3C, 0x00 },
    { 0x7E, 0x42, 0x5A, 0x5A, 0x42, 0x7E, 0x00 },
    { 0x3C, 0x24, 0x24, 0x24, 0x24, 0x7E, 0x00 },
    { 0x3C, 0x42, 0x4A, 0x52, 0x42, 0x3C, 0x00 },
    { 0x18, 0x18, 0x5A, 0x7E, 0x18, 0x24, 0x24 },
    { 0x18, 0x3C, 0x5A, 0x18, 0x18, 0x18, 0x00 },
    { 0x18, 0x18, 0x18, 0x18, 0x5A, 0x3C, 0x18 },
    { 0x18, 0x24, 0x42, 0x42, 0x24, 0x18, 0x00 },
    { 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x00 },
};

static void draw_icon(struct fb *fb, int x, int y, enum screen_icon icon, uint16_t color) {
    const uint8_t *rows = icon_rows[icon];
    for (int row = 0; row < 7; row++) {
        for (int col = 0; col < 8; col++) {
            if (rows[row] & (1u << (7 - col))) {
                draw_pixel(fb, x + col, y + row, color);
            }
        }
    }
}

static void draw_action_glyph(struct fb *fb) {
    for (double angle = 0.0; angle < 2.0 * M_PI; angle += 0.2) {
        int x = (int)(151 + cos(angle) * 4);
        int y = (int)(6 + sin(angle) * 4);
        draw_pixel(fb, x, y, WHITE);
    }
    draw_rect(fb, 150, 5, 3, 3, WHITE);
}

void screen_chrome_begin(struct fb *fb, const char *title) {
    fb_clear(fb, BLACK);
    draw_text_fit(fb, 5, 1, title, 130, false, MUTED);
    draw_action_glyph(fb);
}

void screen_chrome_divider(struct fb *fb) {
    draw_hline(fb, 0, 33, FB_WIDTH, DIVIDER);
}

void screen_chrome_footer(struct fb *fb, const struct screen_metric metrics[3]) {
    screen_chrome_divider(fb);
    draw_vline(fb, 53, 36, 22, DIVIDER);
    draw_vline(fb, 106, 36, 22, DIVIDER);

    for (int index = 0; index < 3; index++) {
        int x = index * 53;
        draw_icon(fb, x + 4, 38, metrics[index].icon, MUTED);
        draw_text_fit(fb, x + 15, 36, metrics[index].label, 36, false, MUTED);
        draw_text_fit(fb, x + 15, 47, metrics[index].value, 36, false, WHITE);
    }
}