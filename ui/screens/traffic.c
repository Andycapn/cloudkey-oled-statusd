#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../fb/draw.h"
#include "../../core/state/state.h"
#include "../../core/state/libre_config.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

static void traffic_screen_render(struct screen *s, struct fb *fb) {
    (void)s;
    const struct libre_stats *l = state_libre();
    char buf[64];

    draw_text(fb, 10, 2, "Core Traffic", WHITE);

    // Draw graph area
    // x: 10 to 150 (width 140)
    // y: 12 to 46 (height 34)
    draw_rect(fb, 10, 12, 140, 34, WHITE); // Border
    draw_hline(fb, 11, 29, 138, WHITE);   // Zero line

    uint64_t max_val = 1000000; // Start with 1Mbps as baseline
    for (int i = 0; i < 60; i++) {
        if (l->traffic_history_in[i] > max_val) max_val = l->traffic_history_in[i];
        if (l->traffic_history_out[i] > max_val) max_val = l->traffic_history_out[i];
    }

    // Draw history
    for (int i = 0; i < 60; i++) {
        int x = 11 + (i * 138 / 60);

        // Map height to half-graph (16 pixels each side of zero line)
        int h_in = (l->traffic_history_in[i] * 16 / max_val);
        int h_out = (l->traffic_history_out[i] * 16 / max_val);

        if (h_in > 0) {
            draw_vline(fb, x, 28 - h_in + 1, h_in, WHITE);
        }
        if (h_out > 0) {
            draw_vline(fb, x, 30, h_out, WHITE);
        }
    }

    // Current values
    double in_mbps = l->traffic_in / 1000000.0;
    double out_mbps = l->traffic_out / 1000000.0;
    snprintf(buf, sizeof(buf), "I:%.1f O:%.1f (Mbps)", in_mbps, out_mbps);
    draw_text(fb, 10, 48, buf, WHITE);
}

static void traffic_screen_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
}

static void traffic_screen_destroy(struct screen *s) {
    free(s);
}

struct screen *traffic_screen_create(void) {
    struct screen *s = calloc(1, sizeof(struct screen));
    s->render = traffic_screen_render;
    s->update = traffic_screen_update;
    s->destroy = traffic_screen_destroy;
    return s;
}
