#include "../ui_manager.h"
#include "../screen_chrome.h"
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

struct libre_priv {
    struct bitmap net_up;
    struct bitmap net_down;
};

static void libre_screen_render(struct screen *s, struct fb *fb) {
    struct libre_priv *priv = s->priv;
    const struct libre_stats *l = state_libre();
    char nodes[12], down[12], warning[12];

    screen_chrome_begin(fb, "LIBRENMS");
    struct bitmap *icon = l->reachable ? &priv->net_up : &priv->net_down;
    draw_bitmap(fb, 8, 14, icon, WHITE);
    if (l->reachable) {
        snprintf(nodes, sizeof(nodes), "%d", l->devices_up);
        snprintf(down, sizeof(down), "%d", l->devices_down);
        snprintf(warning, sizeof(warning), "%d", l->devices_warning);
        draw_text_large(fb, 31, 14, nodes, WHITE);
        draw_text(fb, 58, 19, "NODES UP", 0x7BEF);
    } else {
        snprintf(nodes, sizeof(nodes), "--");
        snprintf(down, sizeof(down), "--");
        snprintf(warning, sizeof(warning), "--");
        draw_text_large(fb, 31, 14, "OFFLINE", WHITE);
    }
    const struct screen_metric metrics[3] = {
        { SCREEN_ICON_CLOUD, "API", l->reachable ? "UP" : "DOWN" },
        { SCREEN_ICON_ALERT, "DOWN", down },
        { SCREEN_ICON_ALERT, "WARN", warning },
    };
    screen_chrome_footer(fb, metrics);
}

static void libre_screen_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
}

static void libre_screen_destroy(struct screen *s) {
    struct libre_priv *priv = s->priv;
    free_bitmap(&priv->net_up);
    free_bitmap(&priv->net_down);
    free(priv);
    free(s);
}

struct screen *libre_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));
    struct libre_priv *priv = calloc(1, sizeof(struct libre_priv));

    load_bitmap(ICON_PATH "net_up_16x16.bin", 16, 16, &priv->net_up);
    load_bitmap(ICON_PATH "net_down_16x16.bin", 16, 16, &priv->net_down);

    s->priv = priv;
    s->render = libre_screen_render;
    s->update = libre_screen_update;
    s->destroy = libre_screen_destroy;

    return s;
}
