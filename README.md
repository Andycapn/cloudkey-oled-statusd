# CloudKey OLED Status Daemon (`oled-statusd`)

A lightweight daemon to manage the OLED display on UniFi CloudKey hardware (Gen2/Gen2 Plus). It features a modular, declarative UI framework and a local simulator for rapid development.

## Table of Contents
- [Features](#features)
- [Development Environment](#development-environment)
  - [Prerequisites](#prerequisites)
  - [Using the Simulator](#using-the-simulator)
- [UI Development](#ui-development)
  - [Creating a New Screen](#creating-a-new-screen)
  - [Transitions](#transitions)
- [Production Build & Installation](#production-build--installation)
  - [Building for Hardware](#building-for-hardware)
  - [Installation](#installation)
- [System Architecture](#system-architecture)

---

## Features
- **Modular UI**: React-like screen components with lifecycle hooks.
- **Smooth Animations**: Support for Slide and Fade transitions.
- **Local Simulator**: Develop and test UI on your Linux desktop via X11.
- **Hardware Optimized**: Targets `/dev/fb0` on the target device.

---

## Development Environment

### Prerequisites
To develop and run the simulator, you need a Linux environment with the following installed:
- `cmake` (version 3.10 or higher)
- `gcc` or `clang`
- `libx11-dev` (for the simulator)

### Using the Simulator
The simulator allows you to run the application on your computer, opening a 2x scaled window representing the OLED display.

1.  **Configure with Simulator enabled**:
    ```bash
    cmake -DSIMULATOR=ON -B cmake-build-debug
    ```
2.  **Build**:
    ```bash
    cmake --build cmake-build-debug --target oled-statusd
    ```
3.  **Run**:
    ```bash
    ./cmake-build-debug/oled-statusd
    ```

In simulator mode, icons are automatically loaded from the `assets/icons/` directory relative to the project root.

---

## UI Development

### Creating a New Screen
Screens are self-contained modules located in `ui/screens/`. To add a new screen:

1.  **Create a new `.c` file** (e.g., `ui/screens/my_screen.c`).
2.  **Implement the Lifecycle**:
    ```c
    #include "../ui_manager.h"
    #include "../../fb/text.h"
    #include <stdlib.h>

    struct my_priv { int value; };

    static void my_render(struct screen *s, struct fb *fb) {
        draw_text(fb, 10, 10, "MY SCREEN", 0xFFFF);
    }

    static void my_update(struct screen *s, uint32_t dt_ms) {
        // Update state logic here
    }

    static void my_destroy(struct screen *s) {
        free(s->priv);
        free(s);
    }

    struct screen *my_screen_create(void) {
        struct screen *s = malloc(sizeof(struct screen));
        s->priv = malloc(sizeof(struct my_priv));
        s->render = my_render;
        s->update = my_update;
        s->destroy = my_destroy;
        return s;
    }
    ```
3.  **Register it**:
    - Add the creation function prototype to `ui/screens.h`.
    - Add the source file to the `ui` library in `CMakeLists.txt`.
    - Instantiate it in `main.c`.

### Transitions
You can switch screens with animations using `ui_manager_switch_to`:
- `TRANSITION_NONE`
- `TRANSITION_SLIDE_LEFT`
- `TRANSITION_SLIDE_RIGHT`
- `TRANSITION_FADE`

Example:
```c
ui_manager_switch_to(&ui, my_screen_create(), TRANSITION_SLIDE_LEFT, 500); // 500ms duration
```

---

## Production Build & Installation

### Building for Hardware
To generate the binary for the actual CloudKey hardware, disable the simulator. This will target the Linux Framebuffer (`/dev/fb0`).

1.  **Configure for Hardware**:
    ```bash
    cmake -DSIMULATOR=OFF -B cmake-build-production
    ```
2.  **Build**:
    ```bash
    cmake --build cmake-build-production --target oled-statusd
    ```

### Installation
The installation process places the executable in `/usr/local/bin` and assets in `/usr/local/share/oled-status/icons/`.

```bash
sudo cmake --install cmake-build-production
```

**Paths on Target System:**
- Binary: `/usr/local/bin/oled-statusd`
- Assets: `/usr/local/share/oled-status/icons/*.bin`

### Systemd Integration
The binary name `oled-statusd` is chosen for compatibility with standard systemd service unit naming. A separate document will cover the full systemd configuration and hardware setup.

---

## System Architecture
- `fb/`: Framebuffer abstraction (Hardware vs Simulator).
- `ui/`: UI Manager and Screen components.
- `stats/`: System data collection (IP, Hostname, Uptime).
- `assets/`: Fonts and raw bitmap icons.
