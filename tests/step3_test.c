#include "../fb/fb.h"
#include "../fb/draw.h"

#define BLACK 0x0000
#define WHITE 0xFFFF

int main(void) {
    struct fb fb;

    if (fb_init(&fb) != 0)
        return 1;

    fb_clear(&fb, BLACK);

    draw_rect(&fb, 10, 10, 140, 40, WHITE);
    draw_hline(&fb, 0, 0, 160, WHITE);
    draw_hline(&fb, 0, 57, 160, WHITE);
    draw_vline(&fb, 0, 0, 58, WHITE);
    draw_vline(&fb, 159, 0, 58, WHITE);


    fb_present(&fb);
    fb_close(&fb);
    return 0;
}
