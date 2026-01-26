#include "../ui_manager.h"
#include "../../stats/host.h"
#include "../../stats/ip.h"
#include "../render_network.h"
#include <stdlib.h>
#include <string.h>

struct network_screen_priv {
    char hostname[64];
    char ip[32];
    uint32_t last_update_ms;
};

static void network_screen_render(struct screen *s, struct fb *fb) {
    struct network_screen_priv *priv = s->priv;
    render_network_info(fb, priv->hostname, priv->ip);
}

static void network_screen_update(struct screen *s, uint32_t dt_ms) {
    struct network_screen_priv *priv = s->priv;
    priv->last_update_ms += dt_ms;
    if (priv->last_update_ms >= 5000) { // update every 5s
        get_hostname(priv->hostname, sizeof(priv->hostname));
        get_primary_ipv4(priv->ip, sizeof(priv->ip));
        priv->last_update_ms = 0;
    }
}

static void network_screen_destroy(struct screen *s) {
    free(s->priv);
    free(s);
}

struct screen *network_screen_create(void) {
    struct screen *s = malloc(sizeof(struct screen));
    struct network_screen_priv *priv = malloc(sizeof(struct network_screen_priv));
    get_hostname(priv->hostname, sizeof(priv->hostname));
    get_primary_ipv4(priv->ip, sizeof(priv->ip));
    priv->last_update_ms = 0;
    
    s->priv = priv;
    s->render = network_screen_render;
    s->update = network_screen_update;
    s->destroy = network_screen_destroy;
    return s;
}
