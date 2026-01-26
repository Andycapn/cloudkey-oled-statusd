#include "../fb/fb.h"
#include <stdint.h>

#define BLACK 0x0000
#define WHITE 0xFFFF

int main(void) {
    struct fb fb;

    if (fb_init(&fb) != 0)
        return 1;

    fb_clear(&fb, BLACK);

    for (int y = 15; y < 45; y++) {
        for (int x = 30; x < 130; x++) {
            fb.buf[y * FB_WIDTH + x] = WHITE;
        }
    }

    fb_present(&fb);
    fb_close(&fb);
    return 0;
}
