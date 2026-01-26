#pragma once
#include <stdint.h>
#include <stdbool.h>

/* ---------------------------
 * System Health
 * --------------------------- */
enum health_state {
    HEALTH_OK,
    HEALTH_DEGRADED,
    HEALTH_FAILED,
};

struct system_health {
    enum health_state state;
    int cpu_percent;
    int mem_percent;
    int disk_percent;
    bool internet_reachable;
};

/* ---------------------------
 * Uptime & Stability
 * --------------------------- */
struct uptime_stats {
    uint64_t uptime_seconds;
    int reboot_count_24h;
};

/* ---------------------------
 * LibreNMS Summary
 * --------------------------- */
struct libre_stats {
    bool reachable;
    int devices_down;
    int devices_warning;
    
    /* Traffic for core device (in bits/sec) */
    uint64_t traffic_in;
    uint64_t traffic_out;
    uint64_t traffic_history_in[60];
    uint64_t traffic_history_out[60];
};

/* ---------------------------
 * Internet Quality
 * --------------------------- */
struct internet_stats {
    bool reachable;
    int avg_latency_ms;
    int packet_loss_percent;
};

/* ---------------------------
 * Identity & Access
 * --------------------------- */
struct identity_info {
    char hostname[64];
    char ip[32];
    char iface[16];
};

/* ---------------------------
 * State Lifecycle
 * --------------------------- */
void state_init(void);
void state_update(uint32_t dt_ms);

/* ---------------------------
 * Read-only Accessors
 * --------------------------- */
const struct system_health  *state_system(void);
const struct uptime_stats  *state_uptime(void);
const struct libre_stats   *state_libre(void);
const struct internet_stats*state_internet(void);
const struct identity_info *state_identity(void);
