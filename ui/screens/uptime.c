#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

static void uptime_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct uptime_stats *u = state_uptime();

    char buf[32];

    draw_text(fb, 10, 10, "UPTIME", WHITE);

    /* Format uptime */
    uint64_t sec = u->uptime_seconds;
    uint64_t days = sec / 86400;
    sec %= 86400;
    uint64_t hours = sec / 3600;
    sec %= 3600;
    uint64_t mins = sec / 60;

    if (days > 0) {
        snprintf(buf, sizeof(buf), "%luD %luH", days, hours);
    } else {
        snprintf(buf, sizeof(buf), "%luH %luM", hours, mins);
    }

    draw_text(fb, 10, 25, buf, WHITE);

    snprintf(buf, sizeof(buf), "REBOOTS 24H: %d", u->reboot_count_24h);
    draw_text(fb, 10, 40, buf, WHITE);
}

static void uptime_screen_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
}

static void uptime_screen_destroy(struct screen *s) {
    free(s);
}

struct screen *uptime_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));

    s->priv = NULL;
    s->render = uptime_screen_render;
    s->update = uptime_screen_update;
    s->destroy = uptime_screen_destroy;

    return s;
}
