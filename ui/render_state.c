#include "render_state.h"
#include "../fb/draw.h"
#include "../fb/text.h"
#include "../fb/bitmap.h"

#define WHITE 0xFFFF

void render_state(struct fb *fb,
                  enum system_state state,
                  const struct state_icons *icons) {

    const struct bitmap *icon = NULL;
    const char *label = state_to_string(state);

    switch (state) {
    case STATE_BOOTING:  icon = &icons->booting; break;
    case STATE_BOOTED:   icon = &icons->booted; break;
    case STATE_DEGRADED: icon = &icons->degraded; break;
    case STATE_FAILED:   icon = &icons->failed; break;
    }

    // Icon top-left
    if (icon) {
        draw_bitmap(fb, 5, 5, icon, WHITE);
    }

    // Text to the right of icon
    draw_text(fb, 26, 9, label, WHITE);
}
