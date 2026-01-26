#include "fb/fb.h"
#include "fb/bitmap.h"
#include "fb/bitmap_loader.h"
#include "fb/text.h"

#define BLACK 0x0000
#define WHITE 0xFFFF

int main(void) {
    struct fb fb;
    struct bitmap logo;

    if (fb_init(&fb) != 0)
        return 1;

    if (load_bitmap("assets/icons/status_ok_16x16.bin", 16, 16, &logo) != 0)
        return 1;

    fb_clear(&fb, BLACK);

    draw_bitmap(&fb, 5, 5, &logo, WHITE);
    draw_text(&fb, 5, 35, "IP 10.0.0.12", WHITE);
    draw_text(&fb, 5, 45, "HOSTNAME", WHITE);

    fb_present(&fb);

    free_bitmap(&logo);
    fb_close(&fb);
    return 0;
}
