#include "../ui_manager.h"
#include "../screen_chrome.h"
#include "../../fb/text.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

static void uptime_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct uptime_stats *u = state_uptime();
    const struct system_health *health = state_system();

    char buf[32];

    /* Format uptime */
    uint64_t sec = u->uptime_seconds;
    uint64_t days = sec / 86400;
    sec %= 86400;
    uint64_t hours = sec / 3600;
    sec %= 3600;
    uint64_t mins = sec / 60;

    if (days > 0) {
        snprintf(buf, sizeof(buf), "%llud %lluh", (unsigned long long)days, (unsigned long long)hours);
    } else {
        snprintf(buf, sizeof(buf), "%lluh %llum", (unsigned long long)hours, (unsigned long long)mins);
    }

    screen_chrome_begin(fb, "STABILITY");
    draw_text_fit(fb, 8, 14, buf, 144, true, WHITE);

    char reboot_count[12];
    snprintf(reboot_count, sizeof(reboot_count), "%d", u->reboot_count_24h);
    const char *health_label = health->state == HEALTH_OK ? "OK" :
                               health->state == HEALTH_DEGRADED ? "WARN" : "FAIL";
    const struct screen_metric metrics[3] = {
        { SCREEN_ICON_CLOCK, "REBOOTS", reboot_count },
        { SCREEN_ICON_CPU, "SYSTEM", health_label },
        { SCREEN_ICON_NETWORK, "LINK", health->internet_reachable ? "UP" : "DOWN" },
    };
    screen_chrome_footer(fb, metrics);
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
