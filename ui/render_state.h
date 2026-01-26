#pragma once
#include "../fb/fb.h"
#include "../stats/state.h"
#include "state_icons.h"

void render_state(struct fb *fb,
                  enum system_state state,
                  const struct state_icons *icons);
