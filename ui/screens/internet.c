#include "../ui_manager.h"
#include "../screen_chrome.h"
#include "../../fb/text.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

static void internet_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct internet_stats *n = state_internet();
    char latency[12], loss[12];

    screen_chrome_begin(fb, "INTERNET");
    if (n->reachable) {
        snprintf(latency, sizeof(latency), "%d ms", n->avg_latency_ms);
        snprintf(loss, sizeof(loss), "%d%%", n->packet_loss_percent);
        draw_text_large(fb, 8, 14, latency, WHITE);
    } else {
        snprintf(latency, sizeof(latency), "OFFLINE");
        snprintf(loss, sizeof(loss), "100%%");
        draw_text_large(fb, 8, 14, latency, WHITE);
    }
    const struct screen_metric metrics[3] = {
        { SCREEN_ICON_NETWORK, "LINK", n->reachable ? "UP" : "DOWN" },
        { SCREEN_ICON_LATENCY, "LAT", n->reachable ? latency : "--" },
        { SCREEN_ICON_LOSS, "LOSS", loss },
    };
    screen_chrome_footer(fb, metrics);
}

static void internet_screen_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
}

static void internet_screen_destroy(struct screen *s) {
    free(s);
}

struct screen *internet_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));

    s->priv = NULL;
    s->render = internet_screen_render;
    s->update = internet_screen_update;
    s->destroy = internet_screen_destroy;

    return s;
}
