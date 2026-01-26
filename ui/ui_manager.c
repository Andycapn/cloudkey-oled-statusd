#include "ui_manager.h"
#include <stdlib.h>
#include <string.h>

void ui_manager_init(struct ui_manager *mgr, struct fb *fb) {
    memset(mgr, 0, sizeof(struct ui_manager));
    mgr->fb = fb;
    mgr->transition_buf = calloc(FB_WIDTH * FB_HEIGHT, sizeof(uint16_t));
}

void ui_manager_cleanup(struct ui_manager *mgr) {
    if (mgr->current_screen && mgr->current_screen->destroy) {
        mgr->current_screen->destroy(mgr->current_screen);
    }
    if (mgr->next_screen && mgr->next_screen->destroy) {
        mgr->next_screen->destroy(mgr->next_screen);
    }
    free(mgr->transition_buf);
}

void ui_manager_set_screen(struct ui_manager *mgr, struct screen *s) {
    if (mgr->current_screen && mgr->current_screen->destroy) {
        mgr->current_screen->destroy(mgr->current_screen);
    }
    mgr->current_screen = s;
}

void ui_manager_switch_to(struct ui_manager *mgr, struct screen *s, enum transition_type t, uint32_t duration_ms) {
    if (t == TRANSITION_NONE || duration_ms == 0) {
        ui_manager_set_screen(mgr, s);
        return;
    }
    
    if (mgr->next_screen && mgr->next_screen->destroy) {
        mgr->next_screen->destroy(mgr->next_screen);
    }
    
    mgr->next_screen = s;
    mgr->transition = t;
    mgr->transition_duration_ms = duration_ms;
    mgr->transition_elapsed_ms = 0;
}

void ui_manager_update(struct ui_manager *mgr, uint32_t dt_ms) {
    if (mgr->current_screen && mgr->current_screen->update) {
        mgr->current_screen->update(mgr->current_screen, dt_ms);
    }
    
    if (mgr->next_screen) {
        if (mgr->next_screen->update) {
            mgr->next_screen->update(mgr->next_screen, dt_ms);
        }
        
        mgr->transition_elapsed_ms += dt_ms;
        if (mgr->transition_elapsed_ms >= mgr->transition_duration_ms) {
            // Transition finished
            if (mgr->current_screen && mgr->current_screen->destroy) {
                mgr->current_screen->destroy(mgr->current_screen);
            }
            mgr->current_screen = mgr->next_screen;
            mgr->next_screen = NULL;
            mgr->transition = TRANSITION_NONE;
        }
    }
}

static void mix_buffers_fade(uint16_t *dst, uint16_t *b1, uint16_t *b2, float ratio) {
    int count = FB_WIDTH * FB_VISIBLE_HEIGHT;
    uint8_t alpha = (uint8_t)(ratio * 255);
    uint8_t inv_alpha = 255 - alpha;

    for (int i = 0; i < count; i++) {
        uint16_t c1 = b1[i];
        uint16_t c2 = b2[i];

        uint8_t r1 = (c1 >> 11) & 0x1F;
        uint8_t g1 = (c1 >> 5) & 0x3F;
        uint8_t b1_val = c1 & 0x1F;

        uint8_t r2 = (c2 >> 11) & 0x1F;
        uint8_t g2 = (c2 >> 5) & 0x3F;
        uint8_t b2_val = c2 & 0x1F;

        uint8_t r = (r1 * inv_alpha + r2 * alpha) >> 8;
        uint8_t g = (g1 * inv_alpha + g2 * alpha) >> 8;
        uint8_t b = (b1_val * inv_alpha + b2_val * alpha) >> 8;

        dst[i] = (r << 11) | (g << 5) | b;
    }
}

static void mix_buffers_slide(uint16_t *dst, uint16_t *b1, uint16_t *b2, float ratio, bool left) {
    int offset = (int)(ratio * FB_WIDTH);
    if (!left) offset = -offset;

    for (int y = 0; y < FB_VISIBLE_HEIGHT; y++) {
        for (int x = 0; x < FB_WIDTH; x++) {
            int src_x = x + offset;
            uint16_t color;
            if (left) {
                if (src_x < FB_WIDTH) {
                    color = b1[y * FB_WIDTH + src_x];
                } else {
                    color = b2[y * FB_WIDTH + (src_x - FB_WIDTH)];
                }
            } else {
                if (src_x >= 0) {
                    color = b1[y * FB_WIDTH + src_x];
                } else {
                    color = b2[y * FB_WIDTH + (src_x + FB_WIDTH)];
                }
            }
            dst[y * FB_WIDTH + x] = color;
        }
    }
}

void ui_manager_render(struct ui_manager *mgr) {
    if (!mgr->current_screen && !mgr->next_screen) return;

    if (mgr->transition == TRANSITION_NONE || !mgr->next_screen) {
        if (mgr->current_screen) {
            mgr->current_screen->render(mgr->current_screen, mgr->fb);
        }
    } else {
        // We need to render both screens to separate buffers and then mix
        // Since fb->buf is the current output, we can use transition_buf as temporary
        
        uint16_t *screen1_buf = malloc(FB_WIDTH * FB_PHYS_HEIGHT * sizeof(uint16_t));
        uint16_t *screen2_buf = malloc(FB_WIDTH * FB_PHYS_HEIGHT * sizeof(uint16_t));
        
        struct fb tmp_fb1 = { .buf = screen1_buf, .backend = NULL };
        struct fb tmp_fb2 = { .buf = screen2_buf, .backend = NULL };
        
        fb_clear(&tmp_fb1, 0);
        mgr->current_screen->render(mgr->current_screen, &tmp_fb1);
        
        fb_clear(&tmp_fb2, 0);
        mgr->next_screen->render(mgr->next_screen, &tmp_fb2);
        
        float ratio = (float)mgr->transition_elapsed_ms / mgr->transition_duration_ms;
        if (ratio > 1.0f) ratio = 1.0f;

        if (mgr->transition == TRANSITION_FADE) {
            mix_buffers_fade(mgr->fb->buf, screen1_buf, screen2_buf, ratio);
        } else if (mgr->transition == TRANSITION_SLIDE_LEFT) {
            mix_buffers_slide(mgr->fb->buf, screen1_buf, screen2_buf, ratio, true);
        } else if (mgr->transition == TRANSITION_SLIDE_RIGHT) {
            mix_buffers_slide(mgr->fb->buf, screen1_buf, screen2_buf, ratio, false);
        }
        
        free(screen1_buf);
        free(screen2_buf);
    }
}
