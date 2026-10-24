#include "led_controller.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#define LED_BASE "/sys/class/leds"

static enum led_state current_led_state = LED_OFF;
static uint32_t blink_timer = 0;
static bool blue_output = false;
static bool white_output = false;
static struct fb *global_fb = NULL;

#ifdef SIMULATOR
#define COLOR_BLUE  0x0000FF
#define COLOR_WHITE 0xFFFFFF
#define COLOR_OFF   0x000000
#endif

static void write_led(const char *led, const char *prop, const char *val) {
#ifdef SIMULATOR
    (void)led;
    (void)prop;
    (void)val;
#else
    char path[256];
    snprintf(path, sizeof(path), LED_BASE "/%s/%s", led, prop);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "%s", val);
    fclose(f);
#endif
}

static void set_led_outputs(bool blue, bool white) {
    if (blue_output != blue) {
        write_led("blue", "brightness", blue ? "255" : "0");
        blue_output = blue;
    }
    if (white_output != white) {
        write_led("white", "brightness", white ? "255" : "0");
        white_output = white;
    }
#ifdef SIMULATOR
    if (global_fb) {
        global_fb->led_color = blue ? COLOR_BLUE : white ? COLOR_WHITE : COLOR_OFF;
    }
#endif
}

static void leds_off(void) {
    write_led("blue", "trigger", "none");
    write_led("white", "trigger", "none");
    write_led("blue", "brightness", "0");
    write_led("white", "brightness", "0");
    blue_output = false;
    white_output = false;
#ifdef SIMULATOR
    if (global_fb) global_fb->led_color = COLOR_OFF;
#endif
}

static uint32_t led_cycle_period(void) {
    switch (current_led_state) {
        case LED_BOOT: return 1600;
        case LED_WARN: return 5000;
        case LED_DEGRADED:
        case LED_CRITICAL: return 2400;
        default: return 0;
    }
}

static void update_led_pattern(void) {
    bool blue = false;
    bool white = false;

    switch (current_led_state) {
        case LED_BOOT:
            blue = blink_timer < 800;
            break;
        case LED_OK:
            blue = true;
            break;
        case LED_WARN:
            white = blink_timer < 120;
            break;
        case LED_DEGRADED:
            blue = blink_timer < 140;
            white = blink_timer >= 260 && blink_timer < 400;
            break;
        case LED_CRITICAL:
            white = blink_timer < 120 ||
                    (blink_timer >= 240 && blink_timer < 360) ||
                    (blink_timer >= 480 && blink_timer < 600);
            break;
        case LED_OFF:
        default:
            break;
    }
    set_led_outputs(blue, white);
}

void led_init(struct fb *fb) {
    global_fb = fb;
    leds_off();
    write_led("ulogo_ctrl", "trigger", "none");
    write_led("ulogo_ctrl", "brightness", "0");
}

void led_set_state(enum led_state state) {
    if (current_led_state == state) return;

    current_led_state = state;
    blink_timer = 0;
    leds_off();
    update_led_pattern();
}

void led_update(uint32_t dt_ms) {
    uint32_t period = led_cycle_period();
    if (period == 0) {
        return;
    }

    blink_timer = (blink_timer + dt_ms) % period;
    update_led_pattern();
}
