#include "../ui_manager.h"
#include "../../fb/text.h"
#include <stdlib.h>
#include <stdio.h>

#define WHITE 0xFFFF

struct hello_screen_priv {
    int counter;
};

static void hello_screen_render(struct screen *s, struct fb *fb) {
    struct hello_screen_priv *priv = s->priv;
    draw_text(fb, 20, 20, "HELLO WORLD", WHITE);
    
    char buf[32];
    sprintf(buf, "COUNT: %d", priv->counter);
    draw_text(fb, 20, 35, buf, WHITE);
}

static void hello_screen_update(struct screen *s, uint32_t dt_ms) {
    struct hello_screen_priv *priv = s->priv;
    static uint32_t timer = 0;
    timer += dt_ms;
    if (timer >= 1000) {
        priv->counter++;
        timer = 0;
    }
}

static void hello_screen_destroy(struct screen *s) {
    free(s->priv);
    free(s);
}

struct screen *hello_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));
    struct hello_screen_priv *priv = malloc(sizeof(struct hello_screen_priv));
    priv->counter = 0;
    
    s->priv = priv;
    s->render = hello_screen_render;
    s->update = hello_screen_update;
    s->destroy = hello_screen_destroy;
    return s;
}
