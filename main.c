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

static volatile int running = 1;

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

static void apply_led_state_for_runtime(const struct system_health *health, const struct libre_stats *libre, int current_screen_idx) {
    if (current_screen_idx == -1) {
        led_set_state(LED_BOOT);
    } else if (health->state == HEALTH_FAILED) {
        led_set_state(LED_CRITICAL);
    } else if (health->state == HEALTH_DEGRADED) {
        led_set_state(LED_DEGRADED);
    } else if (libre->reachable && (libre->devices_down > 0)) {
        led_set_state(LED_CRITICAL);
    } else if (libre->reachable && (libre->devices_warning > 0)) {
        led_set_state(LED_WARN);
    } else {
        led_set_state(LED_OK);
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
    apply_led_state_for_runtime(health, libre, app->current_screen_idx);

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
