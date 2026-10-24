# CloudKey Open Status (`oled-statusd`)

A small Linux status display for repurposed CloudKey Gen2/Gen2 Plus hardware. It turns the built-in OLED into a glanceable view of the host, network, and self-hosted services, with a local simulator for development.

This project is about the useful Linux appliance behind the screen, not the vendor's management UI. The current integrations cover host health, LibreNMS, internet quality, and core-device traffic. The display is designed to grow alongside a home-lab setup that might also include CUPS, SmokePing, and other services; those are ideas for future integrations, not built-in monitors yet.

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
    - [Quick Deploy on Device](#quick-deploy-on-device)
  - [Installation](#installation)
  - [Configuration (LibreNMS)](#configuration-librenms)
  - [Internet Quality Probing](#internet-quality-probing)
  - [LED Status Indicators](#led-status-indicators)
  - [State Persistence (Reboot Tracking)](#state-persistence-reboot-tracking)
  - [System Architecture](#system-architecture)

---

## Features
- **Open Sans Bitmap Fonts**: Proportional printable-ASCII glyphs in compact and large sizes, with grayscale antialiased edges for the RGB565 framebuffer.
- **Unified OLED Screens**: Shared dark frame, pixel iconography, fitted telemetry labels, and modular footer metrics across the status carousel.
- **Modular UI**: React-like screen components with lifecycle hooks.
- **Smooth Animations**: Support for Slide and Fade transitions.
- **Local Simulator**: Develop and test UI on your Linux desktop via X11.
- **Hardware Optimized**: Targets `/dev/fb0` on the target device with monochrome fills and grayscale antialiased text edges.

---

## Development Environment

### Prerequisites
To develop and run the simulator, you need a Linux environment with the following installed:
- `cmake` (version 3.10 or higher)
- `gcc` or `clang`
- `libx11-dev` (for the simulator)
- Python 3 and Pillow (only if regenerating the bitmap font)

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

The Open Sans source font and its SIL Open Font License are included in `assets/fonts/`. To regenerate the checked-in bitmap tables after changing the source font, run:

```bash
python3 tools/generate_opensans_font.py
```

The splash logo is generated from the included high-resolution source with 4-bit antialiased coverage:

```bash
python3 tools/generate_splash_logo.py
```

The generated font is compiled into the daemon; no font library is needed at runtime. `draw_text` uses the compact size, while `draw_text_large` uses the larger size.

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

### Quick Deploy on Device
After pushing changes and pulling them into the project checkout on the CloudKey, run:

```bash
sudo ./tools/deploy.sh
```

The script configures and builds a release hardware binary on the device, installs it and the icon assets, then restarts `oled-statusd.service`. It leaves `/etc/oled-statusd.conf` untouched. The target needs CMake, a C compiler, and the existing systemd service unit.

To restart a unit with a different name, pass it as the first argument:

```bash
sudo ./tools/deploy.sh my-oled-service.service
```

### Installation
The installation process places the executable in `/usr/local/bin` and assets in `/usr/local/share/oled-status/icons/`.

```bash
sudo cmake --install cmake-build-production
```

**Paths on Target System:**
- Binary: `/usr/local/bin/oled-statusd`
- Assets: `/usr/local/share/oled-status/icons/*.bin`

### Configuration (LibreNMS)
LibreNMS integration is optional. To enable it, create or edit the configuration file.

1.  **For Production (Hardware)**:
    Create/Edit `/etc/oled-statusd.conf`:
    ```ini
    # LibreNMS configuration
    LIBRE_URL=http://your-librenms-ip/api/v0
    LIBRE_TOKEN=your_api_token_here
    # ID or hostname of the network core device to monitor
    LIBRE_CORE_DEVICE=1
    # Optional comma-separated interface names; omit to aggregate all interfaces
    LIBRE_CORE_PORTS=eth0,eth1
    ```

2.  **For Development (Simulator)**:
    Create/Edit `config/oled-statusd.conf` in the project root:
    ```ini
    # LibreNMS configuration
    LIBRE_URL=http://your-librenms-ip/api/v0
    LIBRE_TOKEN=your_api_token_here
    LIBRE_CORE_DEVICE=1
    # Optional comma-separated interface names; omit to aggregate all interfaces
    LIBRE_CORE_PORTS=eth0,eth1
    ```
    *If the token is empty or the file is missing, the LibreNMS screens will show "UNAVAILABLE".*

### Core Traffic Monitoring
When `LIBRE_CORE_DEVICE` is configured, the daemon fetches each interface's detail record and aggregates available port statistics. LibreNMS's device port-list endpoint returns interface names only, so traffic counters are read from the per-interface endpoint.

- **Screen**: "Core Traffic" displays a 60-point history graph.
- **Metrics**: Aggregates available `ifInOctets_rate` and `ifOutOctets_rate` values across the core device's interfaces, converting octets/sec to bits/sec.
- **Visualization**: Solid vertical bars represent Incoming traffic; Dots at the peak represent Outgoing traffic.
- **Unit**: Throughput is displayed in Mbps.

### Internet Quality Probing
The daemon periodically probes internet connectivity to provide real-time latency and packet loss metrics.

- **Target**: `8.8.8.8` (Google DNS)
- **Interval**: Every 5 seconds
- **Method**: ICMP Ping (`ping -c 5`)

If the target is unreachable, the "Internet Quality" screen will display "OFFLINE" and the "System Health" status will be marked as "DEGRADED".

### LED Status Indicators
The daemon controls the CloudKey's RGB LED ring to provide at-a-glance system status:

| LED State | Meaning | Condition |
| :--- | :--- | :--- |
| **Blue (Slow Pulse)** | Booting | Initial splash screen phase |
| **Blue (Solid)** | Healthy | System OK, Internet OK, LibreNMS clean |
| **Brief White Pulse** | Warning | LibreNMS reports warning devices |
| **Blue/White Double Pulse** | Degraded | Internet down, high resource use, or configured LibreNMS API unavailable |
| **Triple White Pulse** | Critical | Host health check fails or LibreNMS reports a device down |

Escalations apply immediately; the LED waits three stable seconds before clearing or lowering an alert state. An unconfigured LibreNMS integration does not count as an outage.

### Systemd Integration
The binary name `oled-statusd` is chosen for compatibility with standard systemd service unit naming. A separate document will cover the full systemd configuration and hardware setup.

### State Persistence (Reboot Tracking)
The daemon tracks system reboots over a 24-hour window. This requires a persistent directory to store boot timestamps.

1.  **Create the persistence directory**:
    ```bash
    sudo mkdir -p /var/lib/oled-statusd
    sudo chown root:root /var/lib/oled-statusd
    sudo chmod 755 /var/lib/oled-statusd
    ```
    *Note: If running as a non-root user (e.g., during testing), ensure the user has write access to this directory.*

2.  **How it works**:
    - On startup, the daemon appends the current system time to `/var/lib/oled-statusd/reboots`.
    - It periodically prunes and counts entries from the last 24 hours to display on the "Uptime & Stability" screen.
    - Restarting the service does **not** increment the count (only system boots do).

---

## System Architecture
- `fb/`: Framebuffer abstraction (Hardware vs Simulator).
- `ui/`: UI Manager, shared OLED screen chrome, and screen components.
- `stats/`: System data collection (IP, Hostname, Uptime).
- `assets/`: Fonts and raw bitmap icons.
