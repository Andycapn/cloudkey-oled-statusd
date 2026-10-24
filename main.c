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

#define FRAME_TICK_MS 10u
#define SPLASH_DURATION_MS 3000u
#define SCREEN_ROTATION_MS 5000u
#define SCREEN_TRANSITION_MS 500u
#define LED_RECOVERY_MS 3000u

static volatile int running = 1;
static enum led_state applied_led_state = LED_OFF;
static enum led_state pending_led_state = LED_OFF;
static uint32_t pending_led_ms = 0;

struct app_runtime {
    struct fb fb;
    struct ui_manager ui;
    uint32_t screen_timer_ms;
    int current_screen_idx;
};

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

static int led_state_severity(enum led_state state) {
    switch (state) {
        case LED_CRITICAL: return 4;
        case LED_DEGRADED: return 3;
        case LED_WARN: return 2;
        case LED_OK: return 1;
        case LED_BOOT:
        case LED_OFF:
        default: return 0;
    }
}

static enum led_state requested_led_state(const struct system_health *health,
                                          const struct libre_stats *libre,
                                          int current_screen_idx) {
    if (current_screen_idx == -1) {
        return LED_BOOT;
    }

    if (health->state == HEALTH_FAILED || (libre->reachable && libre->devices_down > 0)) {
        return LED_CRITICAL;
    }
    if (health->state == HEALTH_DEGRADED ||
        (libre->configured && libre->checked && !libre->reachable)) {
        return LED_DEGRADED;
    }
    if (libre->reachable && libre->devices_warning > 0) {
        return LED_WARN;
    }
    return LED_OK;
}

static void apply_led_state_for_runtime(const struct system_health *health,
                                        const struct libre_stats *libre,
                                        int current_screen_idx, uint32_t dt_ms) {
    enum led_state requested = requested_led_state(health, libre, current_screen_idx);
    if (requested == applied_led_state) {
        pending_led_state = applied_led_state;
        pending_led_ms = 0;
        return;
    }

    if (requested == LED_BOOT || applied_led_state == LED_BOOT ||
        requested == LED_CRITICAL ||
        led_state_severity(requested) > led_state_severity(applied_led_state)) {
        applied_led_state = requested;
        pending_led_state = requested;
        pending_led_ms = 0;
        led_set_state(requested);
        return;
    }

    if (pending_led_state != requested) {
        pending_led_state = requested;
        pending_led_ms = 0;
    }
    if (pending_led_ms < LED_RECOVERY_MS) {
        pending_led_ms += dt_ms;
    }
    if (pending_led_ms >= LED_RECOVERY_MS) {
        applied_led_state = requested;
        pending_led_ms = 0;
        led_set_state(requested);
    }
}

static void switch_screen_for_index(struct ui_manager *ui, int screen_index) {
    if (screen_index == 0) {
        ui_manager_switch_to(ui, dashboard_screen_create(), TRANSITION_SLIDE_LEFT, SCREEN_TRANSITION_MS);
    } else if (screen_index == 1) {
        ui_manager_switch_to(ui, system_health_screen_create(), TRANSITION_SLIDE_LEFT, SCREEN_TRANSITION_MS);
    } else if (screen_index == 2) {
        ui_manager_switch_to(ui, identity_screen_create(), TRANSITION_SLIDE_LEFT, SCREEN_TRANSITION_MS);
    } else if (screen_index == 3) {
        ui_manager_switch_to(ui, uptime_screen_create(), TRANSITION_SLIDE_LEFT, SCREEN_TRANSITION_MS);
    } else if (screen_index == 4) {
        ui_manager_switch_to(ui, libre_screen_create(), TRANSITION_SLIDE_LEFT, SCREEN_TRANSITION_MS);
    } else if (screen_index == 5) {
        ui_manager_switch_to(ui, internet_screen_create(), TRANSITION_SLIDE_LEFT, SCREEN_TRANSITION_MS);
    } else if (screen_index == 6) {
        ui_manager_switch_to(ui, traffic_screen_create(), TRANSITION_SLIDE_LEFT, SCREEN_TRANSITION_MS);
    }
}

static void update_screen_rotation(struct app_runtime *app) {
    if (app->current_screen_idx == -1) {
        if (app->screen_timer_ms >= SPLASH_DURATION_MS) {
            app->screen_timer_ms = 0;
            app->current_screen_idx = 0;
            ui_manager_switch_to(&app->ui, dashboard_screen_create(), TRANSITION_FADE, SCREEN_TRANSITION_MS);
        }
        return;
    }

    if (app->screen_timer_ms < SCREEN_ROTATION_MS) {
        return;
    }

    app->screen_timer_ms = 0;
    app->current_screen_idx = (app->current_screen_idx + 1) % 7;
    switch_screen_for_index(&app->ui, app->current_screen_idx);
}

static void process_frame(struct app_runtime *app) {
    fb_clear(&app->fb, BLACK);

    state_update(FRAME_TICK_MS);
    led_update(FRAME_TICK_MS);
    ui_manager_update(&app->ui, FRAME_TICK_MS);
    ui_manager_render(&app->ui);

    fb_present(&app->fb);

    const struct system_health *health = state_system();
    const struct libre_stats *libre = state_libre();
    apply_led_state_for_runtime(health, libre, app->current_screen_idx, FRAME_TICK_MS);

    app->screen_timer_ms += FRAME_TICK_MS;
    update_screen_rotation(app);
}

int main(void) {
    struct app_runtime app = {
        .screen_timer_ms = 0,
        .current_screen_idx = -1,
    };

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (fb_init(&app.fb) != 0) {
        fprintf(stderr, "CRITICAL: Failed to initialize framebuffer\n");
        return 1;
    }

    ui_manager_init(&app.ui, &app.fb);
    ui_manager_set_screen(&app.ui, splash_screen_create());

    state_init();
    led_init(&app.fb);

    while (running) {
        if (fb_should_close(&app.fb)) {
            running = 0;
            break;
        }

        process_frame(&app);

        struct timespec delay = {
            .tv_sec = 0,
            .tv_nsec = FRAME_TICK_MS * 1000 * 1000
        };
        nanosleep(&delay, NULL);
    }

    ui_manager_cleanup(&app.ui);
    fb_close(&app.fb);
    return 0;
}
