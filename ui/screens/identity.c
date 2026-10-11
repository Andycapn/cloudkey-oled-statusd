#include "../ui_manager.h"
#include "../../fb/text.h"
#include "../../fb/draw.h"
#include "../../core/state/state.h"
#include <stdlib.h>

#define WHITE 0xFFFF
#define BLACK 0x0000

static void identity_screen_render(struct screen *s, struct fb *fb) {
    (void)s;

    const struct identity_info *id = state_identity();

    draw_rect(fb, 0, 0, 160, 58, BLACK);
    draw_rect(fb, 0, 0, 160, 10, WHITE);
    draw_text(fb, 4, 2, "DEV", BLACK);

    draw_text(fb, 6, 16, id->hostname, WHITE);
    draw_text(fb, 6, 28, "IP", WHITE);
    draw_text(fb, 28, 28, id->ip, WHITE);
    draw_text(fb, 6, 40, "IF", WHITE);
    draw_text(fb, 28, 40, id->iface, WHITE);
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
