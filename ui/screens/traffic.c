#include "../ui_manager.h"
#include "../screen_chrome.h"
#include "../../fb/text.h"
#include "../../fb/draw.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF
#define MUTED 0x7BEF
#define DIVIDER 0x4208

static void draw_traffic_sparkline(struct fb *fb, const struct libre_stats *stats) {
    if (!stats->core_traffic_available) {
        return;
    }

    uint64_t maximum = 1;
    for (int index = 0; index < 60; index++) {
        uint64_t total = stats->traffic_history_in[index] + stats->traffic_history_out[index];
        if (total > maximum) {
            maximum = total;
        }
    }

    int previous_y = 29;
    for (int index = 0; index < 60; index++) {
        uint64_t total = stats->traffic_history_in[index] + stats->traffic_history_out[index];
        int x = 100 + index * 53 / 59;
        int y = 30 - (int)(total * 15 / maximum);
        int top = y < previous_y ? y : previous_y;
        int height = abs(previous_y - y) + 1;
        draw_vline(fb, x, top, height, MUTED);
        previous_y = y;
    }
}

static void traffic_screen_render(struct screen *s, struct fb *fb) {
    (void)s;
    const struct libre_stats *l = state_libre();
    char total[16], in[16], out[16];

    screen_chrome_begin(fb, "CORE TRAFFIC");
    if (l->core_traffic_available) {
        double total_mbps = (l->traffic_in + l->traffic_out) / 1000000.0;
        snprintf(total, sizeof(total), "%.1fM", total_mbps);
        snprintf(in, sizeof(in), "%.1fM", l->traffic_in / 1000000.0);
        snprintf(out, sizeof(out), "%.1fM", l->traffic_out / 1000000.0);
        draw_text_large(fb, 8, 14, total, WHITE);
    } else {
        snprintf(total, sizeof(total), "--");
        snprintf(in, sizeof(in), "--");
        snprintf(out, sizeof(out), "--");
        draw_text_large(fb, 8, 14, total, WHITE);
    }
    draw_text(fb, 9, 27, "TOTAL MBPS", MUTED);
    draw_traffic_sparkline(fb, l);

    const struct screen_metric metrics[3] = {
        { SCREEN_ICON_DOWNLOAD, "IN", in },
        { SCREEN_ICON_UPLOAD, "OUT", out },
        { SCREEN_ICON_NETWORK, "CORE", l->core_traffic_available ? "UP" : "--" },
    };
    screen_chrome_footer(fb, metrics);
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
