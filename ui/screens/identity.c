#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../core/state/state.h"
#include <stdlib.h>

#define WHITE 0xFFFF

static void identity_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct identity_info *id = state_identity();

    draw_text(fb, 10, 10, "Device", WHITE);
    draw_text(fb, 10, 25, id->hostname, WHITE);

    draw_text(fb, 10, 40, "IP:", WHITE);
    draw_text(fb, 40, 40, id->ip, WHITE);
}

static void identity_screen_update(struct screen *s, uint32_t dt_ms) {
    (void)s;
    (void)dt_ms;
    /* No per-screen logic needed */
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
