#include "../ui_manager.h"
#include "../screen_chrome.h"
#include "../../fb/text.h"
#include "../../core/state/state.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

static void system_health_render(struct screen *s, struct fb *fb) {
    const struct system_health *h = state_system();
    char cpu[8], memory[8], disk[8];
    const char *state = h->state == HEALTH_OK ? "HEALTHY" :
                        h->state == HEALTH_DEGRADED ? "DEGRADED" : "FAILED";

    screen_chrome_begin(fb, "SYSTEM");
    draw_text_large(fb, 8, 14, state, WHITE);

    snprintf(cpu, sizeof(cpu), h->cpu_percent < 0 ? "--" : "%d%%", h->cpu_percent);
    snprintf(memory, sizeof(memory), h->mem_percent < 0 ? "--" : "%d%%", h->mem_percent);
    snprintf(disk, sizeof(disk), h->disk_percent < 0 ? "--" : "%d%%", h->disk_percent);
    const struct screen_metric metrics[3] = {
        { SCREEN_ICON_CPU, "CPU", cpu },
        { SCREEN_ICON_MEMORY, "MEM", memory },
        { SCREEN_ICON_DISK, "DISK", disk },
    };
    screen_chrome_footer(fb, metrics);
}

static void system_health_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
}

static void system_health_destroy(struct screen *s) {
    free(s);
}

struct screen *system_health_screen_create(void) {
    struct screen *s = calloc(1, sizeof(struct screen));
    if (!s) {
        return NULL;
    }

    s->priv = NULL;
    s->render = system_health_render;
    s->update = system_health_update;
    s->destroy = system_health_destroy;

    return s;
}
