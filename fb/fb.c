#include "fb.h"
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define FB_VISIBLE_SIZE (FB_WIDTH * FB_VISIBLE_HEIGHT * 2)
#define FB_PHYS_SIZE (FB_WIDTH * FB_PHYS_HEIGHT * 2)

int fb_init(struct fb *fb) {
    fb->fd = open("/dev/fb0", O_RDWR);
    if (fb->fd < 0) {
        perror("open /dev/fb0");
        return -1;
    }

    fb->buf = calloc(FB_WIDTH * FB_PHYS_HEIGHT, sizeof(uint16_t));
    if (!fb->buf) {
        perror("malloc framebuffer");
        close(fb->fd);
        return -1;
    }

    return 0;
}

void fb_clear(struct fb *fb, uint16_t color) {
    for (int y = 0; y < FB_VISIBLE_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            fb->buf[y * FB_WIDTH + x] = color;
        }
    }
}


void fb_present(struct fb *fb) {
    ssize_t written = pwrite(fb->fd, fb->buf, FB_PHYS_SIZE, 0);
    if (written != FB_PHYS_SIZE) {
        perror("write framebuffer");
    }
}

void fb_close(struct fb *fb) {
    if (fb->buf) free(fb->buf);
    if (fb->fd >= 0) close(fb->fd);
}
