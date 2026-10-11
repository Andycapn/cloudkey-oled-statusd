#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../fb/draw.h"
#include "../../fb/bitmap.h"
#include "../../fb/bitmap_loader.h"
#include "../../core/state/state.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF
#define BLACK 0x0000

#ifndef ICON_PATH
#define ICON_PATH "/usr/local/share/oled-status/icons/"
#endif

struct health_priv {
    struct bitmap status_ok;
    struct bitmap status_degraded;
    struct bitmap status_failed;
    struct bitmap net_up;
    struct bitmap net_down;
};

static void draw_status_badge(struct fb *fb, int x, int y, const char *label) {
    draw_rect(fb, x, y, 28, 8, WHITE);
    draw_text(fb, x + 4, y + 1, label, BLACK);
}

static void draw_status_ring(struct fb *fb, int cx, int cy, int radius, uint16_t color) {
    for (double angle = 0.0; angle < 2.0 * M_PI; angle += 0.12) {
        int x = (int)(cx + cos(angle) * radius);
        int y = (int)(cy + sin(angle) * radius);
        draw_pixel(fb, x, y, color);
    }
}

static void system_health_render(struct screen *s, struct fb *fb) {
    struct health_priv *priv = s->priv;
    const struct system_health *h = state_system();
    char buf[24];

    draw_rect(fb, 0, 0, FB_WIDTH, FB_VISIBLE_HEIGHT, BLACK);
    draw_rect(fb, 0, 0, FB_WIDTH, 10, WHITE);
    draw_text(fb, 4, 2, "SYS", BLACK);

    if (h->state == HEALTH_OK) {
        draw_status_badge(fb, 118, 1, "OK");
    } else if (h->state == HEALTH_DEGRADED) {
        draw_status_badge(fb, 110, 1, "WARN");
    } else {
        draw_status_badge(fb, 113, 1, "FAIL");
    }

    draw_status_ring(fb, 130, 25, 10, WHITE);
    draw_rect(fb, 124, 19, 12, 12, BLACK);

    draw_text(fb, 8, 18, "400", WHITE);
    draw_text(fb, 42, 18, "Mbps", WHITE);

    snprintf(buf, sizeof(buf), "CPU %d%%", h->cpu_percent);
    draw_rect(fb, 6, 32, 46, 8, WHITE);
    draw_text(fb, 10, 33, buf, BLACK);

    snprintf(buf, sizeof(buf), "MEM %d%%", h->mem_percent);
    draw_rect(fb, 6, 42, 48, 8, WHITE);
    draw_text(fb, 10, 43, buf, BLACK);

    snprintf(buf, sizeof(buf), "DISK %d%%", h->disk_percent);
    draw_rect(fb, 6, 52, 54, 8, WHITE);
    draw_text(fb, 10, 53, buf, BLACK);

    if (h->internet_reachable) {
        draw_text(fb, 100, 34, "NET OK", WHITE);
    } else {
        draw_text(fb, 96, 34, "NET OFF", WHITE);
    }

    (void)priv;
}

static void system_health_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
}

static void system_health_destroy(struct screen *s) {
    struct health_priv *priv = s->priv;
    free_bitmap(&priv->status_ok);
    free_bitmap(&priv->status_degraded);
    free_bitmap(&priv->status_failed);
    free_bitmap(&priv->net_up);
    free_bitmap(&priv->net_down);
    free(priv);
    free(s);
}

struct screen *system_health_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));
    struct health_priv *priv = calloc(1, sizeof(struct health_priv));

    load_bitmap(ICON_PATH "status_ok_16x16.bin", 16, 16, &priv->status_ok);
    load_bitmap(ICON_PATH "status_degraded_16x16.bin", 16, 16, &priv->status_degraded);
    load_bitmap(ICON_PATH "status_failed_16x16.bin", 16, 16, &priv->status_failed);
    load_bitmap(ICON_PATH "net_up_16x16.bin", 16, 16, &priv->net_up);
    load_bitmap(ICON_PATH "net_down_16x16.bin", 16, 16, &priv->net_down);

    s->priv = priv;
    s->render = system_health_render;
    s->update = system_health_update;
    s->destroy = system_health_destroy;

    return s;
}
