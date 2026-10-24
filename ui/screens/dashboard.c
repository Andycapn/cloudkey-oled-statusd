#include "../ui_manager.h"
#include "../screen_chrome.h"
#include "../../fb/text.h"
#include "../../fb/bitmap.h"
#include "../../fb/bitmap_loader.h"
#include "../../core/state/state.h"
#include "../../core/state/libre_config.h"
#include <stdio.h>
#include <stdlib.h>

#define WHITE 0xFFFF

#ifndef ICON_PATH
#define ICON_PATH "/usr/local/share/oled-status/icons/"
#endif

struct dashboard_priv {
    struct bitmap net_up;
    struct bitmap net_down;
};

static void dashboard_render(struct screen *s, struct fb *fb) {
    struct dashboard_priv *priv = s->priv;
    const struct system_health *health = state_system();
    const struct libre_stats *libre = state_libre();
    bool libre_configured = libre_url()[0] && libre_token()[0];
    char throughput[24];
    char device_count[12];
    char alert_count[12];

    screen_chrome_begin(fb, "OPEN CLOUDKEY");

    struct bitmap *network_icon = health->internet_reachable ? &priv->net_up : &priv->net_down;
    draw_bitmap(fb, 6, 15, network_icon, WHITE);

    if (!libre->core_traffic_available) {
        snprintf(throughput, sizeof(throughput), "-- Mbps");
    } else {
        double mbps = libre->traffic_in / 1000000.0;
        if (mbps >= 1000.0) {
            snprintf(throughput, sizeof(throughput), "IN %.1f Gbps", mbps / 1000.0);
        } else {
            snprintf(throughput, sizeof(throughput), "IN %.1f Mbps", mbps);
        }
    }
    if (libre->reachable) {
        snprintf(device_count, sizeof(device_count), "%d", libre->devices_up);
        snprintf(alert_count, sizeof(alert_count), "%d",
                 libre->devices_down + libre->devices_warning);
    } else {
        snprintf(device_count, sizeof(device_count), "--");
        snprintf(alert_count, sizeof(alert_count), "--");
    }

    draw_text_fit(fb, 28, 15, throughput, 124, true, WHITE);
    const char *cloud_status = !libre_configured ? "--" : libre->reachable ? "UP" : "DOWN";
    const struct screen_metric metrics[3] = {
        { SCREEN_ICON_CLOUD, "NMS", cloud_status },
        { SCREEN_ICON_DEVICE, "NODES", device_count },
        { SCREEN_ICON_ALERT, "ALERTS", alert_count },
    };
    screen_chrome_footer(fb, metrics);
}

static void dashboard_destroy(struct screen *s) {
    struct dashboard_priv *priv = s->priv;
    free_bitmap(&priv->net_up);
    free_bitmap(&priv->net_down);
    free(priv);
    free(s);
}

struct screen *dashboard_screen_create(void) {
    struct screen *s = calloc(1, sizeof(struct screen));
    struct dashboard_priv *priv = calloc(1, sizeof(struct dashboard_priv));

    if (!s || !priv) {
        free(s);
        free(priv);
        return NULL;
    }

    load_bitmap(ICON_PATH "net_up_16x16.bin", 16, 16, &priv->net_up);
    load_bitmap(ICON_PATH "net_down_16x16.bin", 16, 16, &priv->net_down);

    s->priv = priv;
    s->render = dashboard_render;
    s->update = NULL;
    s->destroy = dashboard_destroy;
    return s;
}