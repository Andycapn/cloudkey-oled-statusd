#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../fb/bitmap.h"
#include "../../fb/bitmap_loader.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

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

static void system_health_render(struct screen *s, struct fb *fb) {
    struct health_priv *priv = s->priv;
    const struct system_health *h = state_system();
    char buf[32];

    draw_text(fb, 10, 5, "SYSTEM HEALTH", WHITE);

    // Status Icon and Label
    // struct bitmap *status_icon = &priv->status_ok;
    // if (h->state == HEALTH_DEGRADED) status_icon = &priv->status_degraded;
    // else if (h->state == HEALTH_FAILED) status_icon = &priv->status_failed;
    //
    // draw_bitmap(fb, 10, 18, status_icon, WHITE);
    
    const char *status_str = "OK";
    if (h->state == HEALTH_DEGRADED) status_str = "DEGRADED";
    else if (h->state == HEALTH_FAILED) status_str = "FAILED";
    
    snprintf(buf, sizeof(buf), "STATUS: %s", status_str);
    draw_text(fb, 10, 22, buf, WHITE);

    // Stats
    snprintf(buf, sizeof(buf), "CPU: %d%%", h->cpu_percent);
    draw_text(fb, 10, 38, buf, WHITE);

    snprintf(buf, sizeof(buf), "MEM: %d%%", h->mem_percent);
    draw_text(fb, 85, 38, buf, WHITE);

    snprintf(buf, sizeof(buf), "DISK: %d%%", h->disk_percent);
    draw_text(fb, 10, 48, buf, WHITE);

    // // Net Icon
    // struct bitmap *net_icon = h->internet_reachable ? &priv->net_up : &priv->net_down;
    // draw_bitmap(fb, 85, 46, net_icon, WHITE);
    draw_text(fb, 85, 48, h->internet_reachable ? "NET: OK" : "NET: FAIL", WHITE);
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
