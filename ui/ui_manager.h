#pragma once

#include <stdint.h>
#include <stdbool.h>
#include "fb.h"

struct screen;
struct ui_manager;

typedef void (*screen_render_fn)(struct screen *s, struct fb *fb);
typedef void (*screen_update_fn)(struct screen *s, uint32_t dt_ms);
typedef void (*screen_destroy_fn)(struct screen *s);

struct screen {
    void *priv;
    screen_render_fn render;
    screen_update_fn update;
    screen_destroy_fn destroy;
};

enum transition_type {
    TRANSITION_NONE,
    TRANSITION_SLIDE_LEFT,
    TRANSITION_SLIDE_RIGHT,
    TRANSITION_FADE
};

struct ui_manager {
    struct screen *current_screen;
    struct screen *next_screen;
    
    enum transition_type transition;
    uint32_t transition_duration_ms;
    uint32_t transition_elapsed_ms;
    
    struct fb *fb;
    uint16_t *transition_buf; // Temporary buffer for transitions
};

void ui_manager_init(struct ui_manager *mgr, struct fb *fb);
void ui_manager_cleanup(struct ui_manager *mgr);

void ui_manager_set_screen(struct ui_manager *mgr, struct screen *s);
void ui_manager_switch_to(struct ui_manager *mgr, struct screen *s, enum transition_type t, uint32_t duration_ms);

void ui_manager_update(struct ui_manager *mgr, uint32_t dt_ms);
void ui_manager_render(struct ui_manager *mgr);
