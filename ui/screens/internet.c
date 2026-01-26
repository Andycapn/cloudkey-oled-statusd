#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

static void internet_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct internet_stats *n = state_internet();
    char buf[32];

    draw_text(fb, 10, 10, "Internet", WHITE);

    if (!n->reachable) {
        draw_text(fb, 10, 30, "down", WHITE);
        return;
    }

    snprintf(buf, sizeof(buf), "latency: %dms",
             n->avg_latency_ms >= 0 ? n->avg_latency_ms : 0);
    draw_text(fb, 10, 28, buf, WHITE);

    snprintf(buf, sizeof(buf), "loss: %d%%", n->packet_loss_percent);
    draw_text(fb, 10, 42, buf, WHITE);
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
