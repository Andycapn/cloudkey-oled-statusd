#pragma once
#include "../fb/bitmap.h"

struct state_icons {
    struct bitmap booting;
    struct bitmap booted;
    struct bitmap degraded;
    struct bitmap failed;
};

int load_state_icons(struct state_icons *icons);
void free_state_icons(struct state_icons *icons);
