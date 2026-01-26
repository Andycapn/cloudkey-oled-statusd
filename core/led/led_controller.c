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
static bool blink_phase = false;
static struct fb *global_fb = NULL;

#ifdef SIMULATOR
#define COLOR_BLUE  0x0000FF
#define COLOR_WHITE 0xFFFFFF
#define COLOR_OFF   0x000000
#endif

static void update_sim_led(void) {
#ifdef SIMULATOR
    if (!global_fb) return;

    switch (current_led_state) {
        case LED_BOOT:
            global_fb->led_color = blink_phase ? COLOR_BLUE : COLOR_OFF;
            break;
        case LED_OK:
            global_fb->led_color = COLOR_BLUE;
            break;
        case LED_WARN:
            global_fb->led_color = COLOR_WHITE;
            break;
        case LED_CRITICAL:
            global_fb->led_color = blink_phase ? COLOR_WHITE : COLOR_OFF;
            break;
        case LED_DEGRADED:
            global_fb->led_color = blink_phase ? COLOR_BLUE : COLOR_WHITE;
            break;
        case LED_OFF:
        default:
            global_fb->led_color = COLOR_OFF;
            break;
    }
#endif
}

static void write_led(const char *led, const char *prop, const char *val) {
#ifdef SIMULATOR
    if (!global_fb) return;

    if (strcmp(prop, "brightness") == 0) {
        int brightness = atoi(val);
        if (strcmp(led, "blue") == 0) {
            if (brightness > 0) {
                if (current_led_state == LED_DEGRADED) {
                     global_fb->led_color = COLOR_BLUE;
                } else {
                     global_fb->led_color = COLOR_BLUE;
                }
            } else {
                if (global_fb->led_color == COLOR_BLUE) global_fb->led_color = COLOR_OFF;
            }
        } else if (strcmp(led, "white") == 0) {
            if (brightness > 0) {
                global_fb->led_color = COLOR_WHITE;
            } else {
                if (global_fb->led_color == COLOR_WHITE) global_fb->led_color = COLOR_OFF;
            }
        }
    }
    return;
#else
    char path[256];
    snprintf(path, sizeof(path), LED_BASE "/%s/%s", led, prop);
    FILE *f = fopen(path, "w");
    if (!f) return;
    fprintf(f, "%s", val);
    fclose(f);
#endif
}

static void leds_off(void) {
    write_led("blue", "trigger", "none");
    write_led("white", "trigger", "none");
    write_led("blue", "brightness", "0");
    write_led("white", "brightness", "0");
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
    blink_phase = false;

    leds_off();
    update_sim_led();

    switch (state) {
        case LED_BOOT:
            write_led("blue", "trigger", "timer");
            write_led("blue", "delay_on", "800");
            write_led("blue", "delay_off", "800");
            break;
        case LED_OK:
            write_led("blue", "brightness", "255");
            break;
        case LED_WARN:
            write_led("white", "brightness", "255");
            break;
        case LED_CRITICAL:
            write_led("white", "trigger", "timer");
            write_led("white", "delay_on", "200");
            write_led("white", "delay_off", "200");
            break;
        case LED_DEGRADED:
            // This one needs manual blinking in led_update if we want alternating
            break;
        case LED_OFF:
        default:
            break;
    }
}

void led_update(uint32_t dt_ms) {
    if (current_led_state == LED_BOOT || current_led_state == LED_CRITICAL || current_led_state == LED_DEGRADED) {
        blink_timer += dt_ms;
        uint32_t threshold = (current_led_state == LED_CRITICAL) ? 200 : (current_led_state == LED_BOOT ? 800 : 500);
        
        if (blink_timer >= threshold) {
            blink_timer = 0;
            blink_phase = !blink_phase;
            
            if (current_led_state == LED_BOOT) {
                write_led("blue", "brightness", blink_phase ? "255" : "0");
            } else if (current_led_state == LED_CRITICAL) {
                write_led("white", "brightness", blink_phase ? "255" : "0");
            } else if (current_led_state == LED_DEGRADED) {
                if (blink_phase) {
                    write_led("blue", "brightness", "255");
                    write_led("white", "brightness", "0");
                } else {
                    write_led("blue", "brightness", "0");
                    write_led("white", "brightness", "255");
                }
            }
#ifdef SIMULATOR
            update_sim_led();
#endif
        }
    }
}
