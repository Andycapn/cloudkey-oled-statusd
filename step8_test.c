#include "fb/fb.h"
#include "stats/state.h"
#include "ui/state_icons.h"
#include "ui/render_state.h"

#define BLACK 0x0000

int main(void) {
    struct fb fb;
    struct state_icons icons;

    if (fb_init(&fb) != 0)
        return 1;

    if (load_state_icons(&icons) != 0)
        return 1;

    enum system_state s = get_system_state();

    fb_clear(&fb, BLACK);
    render_state(&fb, s, &icons);
    fb_present(&fb);

    free_state_icons(&icons);
    fb_close(&fb);
    return 0;
}
