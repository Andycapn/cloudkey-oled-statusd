#include "render_network.h"
#include "../fb/text.h"

#define WHITE 0xFFFF

void render_network_info(struct fb *fb,
                         const char *hostname,
                         const char *ip) {
    draw_text(fb, 5, 28, "HOST:", WHITE);
    draw_text(fb, 40, 28, hostname, WHITE);

    draw_text(fb, 5, 40, "IP:", WHITE);
    draw_text(fb, 40, 40, ip, WHITE);
}


