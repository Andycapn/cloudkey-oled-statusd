#include <signal.h>
#include <time.h>
#include <string.h>
#include <stdio.h>

#include "fb/fb.h"
#include "ui/ui_manager.h"
#include "ui/screens.h"

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
    ui_manager_set_screen(&ui, status_screen_create());

    uint32_t screen_timer = 0;
    int current_screen_idx = 0;

    while (running) {
        if (fb_should_close(&fb)) {
            running = 0;
            break;
        }

        fb_clear(&fb, BLACK);
        
        ui_manager_update(&ui, 100);
        ui_manager_render(&ui);
        
        fb_present(&fb);

        screen_timer += 100;
        if (screen_timer >= 5000) {
            screen_timer = 0;
            current_screen_idx = (current_screen_idx + 1) % 3;
            if (current_screen_idx == 0) {
                ui_manager_switch_to(&ui, status_screen_create(), TRANSITION_SLIDE_LEFT, 500);
            } else if (current_screen_idx == 1) {
                ui_manager_switch_to(&ui, network_screen_create(), TRANSITION_SLIDE_RIGHT, 500);
            } else {
                ui_manager_switch_to(&ui, hello_screen_create(), TRANSITION_FADE, 500);
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
