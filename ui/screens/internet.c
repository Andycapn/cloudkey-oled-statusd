#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../fb/draw.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF
#define BLACK 0x0000

static void internet_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct internet_stats *n = state_internet();
    char buf[28];

    draw_rect(fb, 0, 0, 160, 58, BLACK);
    draw_rect(fb, 0, 0, 160, 10, WHITE);
    draw_text(fb, 4, 2, "NET", BLACK);

    if (n->reachable) {
        draw_rect(fb, 92, 1, 48, 8, WHITE);
        draw_text(fb, 100, 2, "UP", BLACK);
        snprintf(buf, sizeof(buf), "LAT %dMS", n->avg_latency_ms);
        draw_text(fb, 6, 18, buf, WHITE);
        snprintf(buf, sizeof(buf), "LOSS %d%%", n->packet_loss_percent);
        draw_text(fb, 6, 30, buf, WHITE);
        draw_text(fb, 6, 42, "QUALITY OK", WHITE);
    } else {
        draw_rect(fb, 86, 1, 60, 8, WHITE);
        draw_text(fb, 92, 2, "DOWN", BLACK);
        draw_text(fb, 6, 20, "LINK LOST", WHITE);
        draw_text(fb, 6, 34, "PING FAIL", WHITE);
        draw_text(fb, 6, 48, "RETRYING", WHITE);
    }
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
