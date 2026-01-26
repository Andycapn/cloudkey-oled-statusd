#include "fb.h"
#include <stdlib.h>
#include <string.h>

struct fb_backend {
    void (*present)(struct fb *fb);
    void (*close)(struct fb *fb);
    bool (*should_close)(struct fb *fb);
    void *priv;
};

#ifdef SIMULATOR
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

struct sim_priv {
    Display *display;
    Window window;
    GC gc;
    XImage *image;
};

static void sim_present(struct fb *fb) {
    struct sim_priv *priv = fb->backend->priv;
    
    for (int y = 0; y < FB_VISIBLE_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            uint16_t pixel = fb->buf[y * FB_WIDTH + x];
            // RGB565 to RGB888
            uint8_t r = ((pixel >> 11) & 0x1F) << 3;
            uint8_t g = ((pixel >> 5) & 0x3F) << 2;
            uint8_t b = (pixel & 0x1F) << 3;
            uint32_t color = (r << 16) | (g << 8) | b;
            
            // 2x2 scaling
            XPutPixel(priv->image, x * 2, y * 2, color);
            XPutPixel(priv->image, x * 2 + 1, y * 2, color);
            XPutPixel(priv->image, x * 2, y * 2 + 1, color);
            XPutPixel(priv->image, x * 2 + 1, y * 2 + 1, color);
        }
    }
    
    XPutImage(priv->display, priv->window, priv->gc, priv->image, 0, 0, 0, 0, FB_WIDTH * 2, FB_VISIBLE_HEIGHT * 2);
    XFlush(priv->display);
}

static void sim_close(struct fb *fb) {
    struct sim_priv *priv = fb->backend->priv;
    XDestroyImage(priv->image);
    XFreeGC(priv->display, priv->gc);
    XDestroyWindow(priv->display, priv->window);
    XCloseDisplay(priv->display);
    free(priv);
    free(fb->backend);
}

static bool sim_should_close(struct fb *fb) {
    struct sim_priv *priv = fb->backend->priv;
    XEvent event;
    while (XPending(priv->display)) {
        XNextEvent(priv->display, &event);
        if (event.type == ClientMessage) {
            return true;
        }
    }
    return false;
}

static int fb_init_sim(struct fb *fb) {
    struct sim_priv *priv = malloc(sizeof(struct sim_priv));
    priv->display = XOpenDisplay(NULL);
    if (!priv->display) {
        free(priv);
        return -1;
    }
    
    int screen = DefaultScreen(priv->display);
    priv->window = XCreateSimpleWindow(priv->display, RootWindow(priv->display, screen), 
                                      10, 10, FB_WIDTH * 2, FB_VISIBLE_HEIGHT * 2, 1,
                                      BlackPixel(priv->display, screen), WhitePixel(priv->display, screen));
    
    Atom wmDeleteMessage = XInternAtom(priv->display, "WM_DELETE_WINDOW", False);
    XSetWMProtocols(priv->display, priv->window, &wmDeleteMessage, 1);

    XSelectInput(priv->display, priv->window, ExposureMask | KeyPressMask);
    XMapWindow(priv->display, priv->window);
    
    priv->gc = XCreateGC(priv->display, priv->window, 0, NULL);
    
    Visual *visual = DefaultVisual(priv->display, screen);
    int depth = DefaultDepth(priv->display, screen);
    
    char *image_data = malloc(FB_WIDTH * 2 * FB_VISIBLE_HEIGHT * 2 * 4);
    priv->image = XCreateImage(priv->display, visual, depth, ZPixmap, 0, image_data, FB_WIDTH * 2, FB_VISIBLE_HEIGHT * 2, 32, 0);
    
    fb->backend = malloc(sizeof(struct fb_backend));
    fb->backend->present = sim_present;
    fb->backend->close = sim_close;
    fb->backend->should_close = sim_should_close;
    fb->backend->priv = priv;
    
    return 0;
}

#else

#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>

#define FB_PHYS_SIZE (FB_WIDTH * FB_PHYS_HEIGHT * 2)

struct hw_priv {
    int fd;
};

static void hw_present(struct fb *fb) {
    struct hw_priv *priv = fb->backend->priv;
    pwrite(priv->fd, fb->buf, FB_PHYS_SIZE, 0);
}

static void hw_close(struct fb *fb) {
    struct hw_priv *priv = fb->backend->priv;
    close(priv->fd);
    free(priv);
    free(fb->backend);
}

static bool hw_should_close(struct fb *fb) {
    (void)fb;
    return false;
}

static int fb_init_hw(struct fb *fb) {
    int fd = open("/dev/fb0", O_RDWR);
    if (fd < 0) return -1;
    
    struct hw_priv *priv = malloc(sizeof(struct hw_priv));
    priv->fd = fd;
    
    fb->backend = malloc(sizeof(struct fb_backend));
    fb->backend->present = hw_present;
    fb->backend->close = hw_close;
    fb->backend->should_close = hw_should_close;
    fb->backend->priv = priv;
    
    return 0;
}
#endif

int fb_init(struct fb *fb) {
    fb->buf = calloc(FB_WIDTH * FB_PHYS_HEIGHT, sizeof(uint16_t));
    if (!fb->buf) return -1;
    
#ifdef SIMULATOR
    if (fb_init_sim(fb) != 0) {
        free(fb->buf);
        return -1;
    }
#else
    if (fb_init_hw(fb) != 0) {
        free(fb->buf);
        return -1;
    }
#endif
    return 0;
}

void fb_clear(struct fb *fb, uint16_t color) {
    for (int i = 0; i < FB_WIDTH * FB_PHYS_HEIGHT; i++) {
        fb->buf[i] = color;
    }
}

void fb_present(struct fb *fb) {
    fb->backend->present(fb);
}

void fb_close(struct fb *fb) {
    if (fb->buf) free(fb->buf);
    if (fb->backend) fb->backend->close(fb);
}

bool fb_should_close(struct fb *fb) {
    return fb->backend->should_close(fb);
}
