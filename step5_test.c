#include "fb/fb.h"
#include "fb/draw.h"
#include "fb/text.h"
#include "fb/bitmap.h"
#include "assets/icon_test.h"

#define BLACK 0x0000
#define WHITE 0xFFFF

int main(void) {
    struct fb fb;
    if (fb_init(&fb) != 0)
        return 1;

    fb_clear(&fb, BLACK);

    draw_text(&fb, 10, 5, "ICON TEST", WHITE);
    draw_bitmap(&fb, 10, 20, &icon_test, WHITE);
    draw_bitmap(&fb, 40, 20, &icon_test, WHITE);

    fb_present(&fb);
    fb_close(&fb);
    return 0;
}
