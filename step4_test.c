#include "fb/fb.h"
#include "fb/text.h"

#define BLACK 0x0000
#define WHITE 0xFFFF

int main(void) {
    struct fb fb;
    if (fb_init(&fb) != 0)
        return 1;

    fb_clear(&fb, BLACK);

    draw_text(&fb, 10, 10, "BOOTING", WHITE);
    draw_text(&fb, 10, 25, "IP 10.0.0.12", WHITE);
    draw_text(&fb, 10, 40, "LIBRE OK", WHITE);

    fb_present(&fb);
    fb_close(&fb);
    return 0;
}

