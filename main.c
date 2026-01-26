#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "fb/fb.h"
#include "ui/ui_manager.h"
#include "ui/screens.h"
#include "core/state/state.h"
#include "core/led/led_controller.h"

#define BLACK 0x0000

static volatile int running = 1;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

int main(void) {
    struct fb fb;
    struct ui_manager ui;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (fb_init(&fb) != 0) {
        fprintf(stderr, "CRITICAL: Failed to initialize framebuffer\n");
        return 1;
    }

    ui_manager_init(&ui, &fb);
    ui_manager_set_screen(&ui, splash_screen_create());

    uint32_t screen_timer = 0;
    int current_screen_idx = -1; // Start at -1 to indicate splash screen

    state_init();
    led_init(&fb);

    while (running) {
        if (fb_should_close(&fb)) {
            running = 0;
            break;
        }

        fb_clear(&fb, BLACK);

        state_update(100);
        led_update(100);
        ui_manager_update(&ui, 100);
        ui_manager_render(&ui);
        
        fb_present(&fb);

        // Update LED based on state
        const struct system_health *h = state_system();
        const struct libre_stats *l = state_libre();

        if (current_screen_idx == -1) {
            led_set_state(LED_BOOT);
        } else if (h->state == HEALTH_FAILED) {
            led_set_state(LED_CRITICAL);
        } else if (h->state == HEALTH_DEGRADED) {
            led_set_state(LED_DEGRADED);
        } else if (l->reachable && (l->devices_down > 0)) {
            led_set_state(LED_CRITICAL);
        } else if (l->reachable && (l->devices_warning > 0)) {
            led_set_state(LED_WARN);
        } else {
            led_set_state(LED_OK);
        }

        screen_timer += 100;
        
        // Splash screen duration (e.g., 3 seconds)
        if (current_screen_idx == -1) {
            if (screen_timer >= 3000) {
                screen_timer = 0;
                current_screen_idx = 0;
                ui_manager_switch_to(&ui, system_health_screen_create(), TRANSITION_FADE, 500);
            }
        } else if (screen_timer >= 5000) {
            screen_timer = 0;
            current_screen_idx = (current_screen_idx + 1) % 5;
            if (current_screen_idx == 0) {
                ui_manager_switch_to(&ui, system_health_screen_create(), TRANSITION_SLIDE_LEFT, 500);
            } else if (current_screen_idx == 1) {
                ui_manager_switch_to(&ui, identity_screen_create(), TRANSITION_SLIDE_LEFT, 500);
            } else if (current_screen_idx == 2) {
                ui_manager_switch_to(&ui, uptime_screen_create(), TRANSITION_SLIDE_LEFT, 500);
            } else if (current_screen_idx == 3) {
                ui_manager_switch_to(&ui, libre_screen_create(), TRANSITION_SLIDE_LEFT, 500);
            } else if (current_screen_idx == 4) {
                ui_manager_switch_to(&ui, internet_screen_create(), TRANSITION_SLIDE_LEFT, 500);
            }
        }

        struct timespec delay = {
            .tv_sec = 0,
            .tv_nsec = 100 * 1000 * 1000  // 100 ms
        };
        nanosleep(&delay, NULL);
    }

    ui_manager_cleanup(&ui);
    fb_close(&fb);
    return 0;
}
