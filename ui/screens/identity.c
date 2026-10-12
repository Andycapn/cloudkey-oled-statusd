#include "../ui_manager.h"
#include "../screen_chrome.h"
#include "../../fb/text.h"
#include "../../core/state/state.h"
#include <stdlib.h>

#define WHITE 0xFFFF

static void identity_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct identity_info *id = state_identity();
    const struct system_health *health = state_system();

    screen_chrome_begin(fb, "DEVICE");
    draw_text_fit(fb, 8, 15, id->ip, 144, true, WHITE);
    const struct screen_metric metrics[3] = {
        { SCREEN_ICON_DEVICE, "HOST", id->hostname },
        { SCREEN_ICON_NETWORK, "IFACE", id->iface },
        { SCREEN_ICON_CLOUD, "LINK", health->internet_reachable ? "UP" : "DOWN" },
    };
    screen_chrome_footer(fb, metrics);
}

static void identity_screen_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
}

static void identity_screen_destroy(struct screen *s) {
    free(s);
}

struct screen *identity_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));

    s->priv = NULL;
    s->render = identity_screen_render;
    s->update = identity_screen_update;
    s->destroy = identity_screen_destroy;

    return s;
}
