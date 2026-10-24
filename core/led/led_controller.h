#pragma once
#include <stdint.h>

enum led_state {
    LED_OFF,
    LED_BOOT,      // Slow blue pulse
    LED_OK,        // Solid blue
    LED_WARN,      // Brief white pulse every five seconds
    LED_CRITICAL,  // Triple white pulse
    LED_DEGRADED   // Blue/white double pulse
};

#include "../../fb/fb.h"

void led_init(struct fb *fb);
void led_set_state(enum led_state state);
void led_update(uint32_t dt_ms);
