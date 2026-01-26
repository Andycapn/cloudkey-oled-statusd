#pragma once
#include <stdint.h>

enum led_state {
    LED_OFF,
    LED_BOOT,      // Slow blue blink
    LED_OK,        // Solid blue
    LED_WARN,      // Solid white
    LED_CRITICAL,  // Fast white blink
    LED_DEGRADED   // Alternating blue/white
};

#include "../../fb/fb.h"

void led_init(struct fb *fb);
void led_set_state(enum led_state state);
void led_update(uint32_t dt_ms);
