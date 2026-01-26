#include <fcntl.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define W 160
#define H 60
#define FB_SIZE (W * H * 2)

/* RGB565 helpers — we will only use black & white */
#define RGB_BLACK 0x0000
#define RGB_WHITE 0xFFFF

int main(void) {
    int fb = open("/dev/fb0", O_RDWR);
    if (fb < 0) {
        perror("open /dev/fb0");
        return 1;
    }

    static uint16_t buf[W * H];

    /* Step 1: clear screen to black */
    for (int i = 0; i < W * H; i++) {
        buf[i] = RGB_BLACK;
    }

    /* Step 2: draw a white rectangle */
    for (int y = 10; y < 50; y++) {
        for (int x = 20; x < 140; x++) {
            buf[y * W + x] = RGB_WHITE;
        }
    }

    /* Step 3: push frame */
    ssize_t written = write(fb, buf, FB_SIZE);
    if (written != FB_SIZE) {
        perror("write framebuffer");
    }

    close(fb);
    return 0;
}
