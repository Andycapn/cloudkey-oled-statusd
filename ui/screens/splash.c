#include "../ui_manager.h"
#include "../../fb/bitmap_loader.h"
#include "../../fb/bitmap.h"
#include <stdlib.h>
#include <stdio.h>

#ifndef ICON_PATH
#define ICON_PATH "/usr/local/share/oled-status/icons/"
#endif

#define WHITE 0xFFFF
#define LOGO_W 80
#define LOGO_H 24

struct splash_screen_priv {
    struct bitmap logo;
};

static void splash_screen_render(struct screen *s, struct fb *fb) {
    struct splash_screen_priv *priv = s->priv;
    // Center the logo
    int x = (FB_WIDTH - LOGO_W) / 2;
    int y = (FB_VISIBLE_HEIGHT - LOGO_H) / 2;
    draw_bitmap(fb, x, y, &priv->logo, WHITE);
}

static void splash_screen_destroy(struct screen *s) {
    struct splash_screen_priv *priv = s->priv;
    free_bitmap(&priv->logo);
    free(priv);
    free(s);
}

struct screen *splash_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));
    struct splash_screen_priv *priv = malloc(sizeof(struct splash_screen_priv));
    
    if (load_bitmap(ICON_PATH "logo_80x24.bin", LOGO_W, LOGO_H, &priv->logo) != 0) {
        fprintf(stderr, "Failed to load splash logo\n");
        // We could still return a screen that does nothing or just text
    }

    s->priv = priv;
    s->render = splash_screen_render;
    s->update = NULL;
    s->destroy = splash_screen_destroy;
    return s;
}
