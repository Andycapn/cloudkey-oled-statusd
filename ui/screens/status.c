#include "../ui_manager.h"
#include "../../stats/state.h"
#include "../state_icons.h"
#include "../render_state.h"
#include <stdlib.h>

struct status_screen_priv {
    struct state_icons icons;
};

static void status_screen_render(struct screen *s, struct fb *fb) {
    struct status_screen_priv *priv = s->priv;
    enum system_state state = get_system_state();
    render_state(fb, state, &priv->icons);
}

static void status_screen_destroy(struct screen *s) {
    struct status_screen_priv *priv = s->priv;
    free_state_icons(&priv->icons);
    free(priv);
    free(s);
}

struct screen *status_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));
    struct status_screen_priv *priv = malloc(sizeof(struct status_screen_priv));
    load_state_icons(&priv->icons);
    
    s->priv = priv;
    s->render = status_screen_render;
    s->update = NULL;
    s->destroy = status_screen_destroy;
    return s;
}
