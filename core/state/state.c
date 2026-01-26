#include "state.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <stdlib.h>

#include "../../stats/host.h"
#include "../../stats/ip.h"
#include "libre_config.h"

/* ---------------------------
 * Static State Storage
 * --------------------------- */

static struct system_health  g_system;
static struct uptime_stats  g_uptime;
static struct libre_stats   g_libre;
static struct internet_stats g_internet;
static struct identity_info g_identity;

static uint32_t identity_timer = 0;
#define IDENTITY_UPDATE_MS 5000

static uint32_t uptime_timer = 0;
#define UPTIME_UPDATE_MS 1000

static uint32_t health_timer = 0;
#define HEALTH_UPDATE_MS 2000

static uint32_t libre_timer = 0;
#define LIBRE_UPDATE_MS 15000

static uint32_t internet_timer = 0;
#define INTERNET_UPDATE_MS 5000

#define REBOOT_FILE "/var/lib/oled-statusd/reboots"
#define SECONDS_24H (24 * 60 * 60)

/* ---------------------------
 * Reboot Tracking Helpers
 * --------------------------- */

static void record_reboot(time_t now) {
    FILE *f = fopen(REBOOT_FILE, "a");
    if (!f)
        return;

    fprintf(f, "%ld\n", now);
    fclose(f);
}

static void poll_libre(void) {
    if (!libre_url()[0] || !libre_token()[0]) {
        g_libre.reachable = false;
        return;
    }

    char cmd[512];
    snprintf(cmd, sizeof(cmd),
        "curl -sf -H \"X-Auth-Token: %s\" \"%s/devices\"",
        libre_token(), libre_url());

    FILE *p = popen(cmd, "r");
    if (!p) {
        g_libre.reachable = false;
        return;
    }

    char buf[1024];
    int down = 0, warn = 0;
    int ok = 0;

    while (fgets(buf, sizeof(buf), p)) {
        if (strstr(buf, "\"status\":\"down\""))
            down++;
        else if (strstr(buf, "\"status\":\"warning\""))
            warn++;
        else if (strstr(buf, "\"status\":\"up\""))
            ok++;
    }

    int rc = pclose(p);
    if (rc != 0) {
        g_libre.reachable = false;
        return;
    }

    g_libre.reachable = true;
    g_libre.devices_down = down;
    g_libre.devices_warning = warn;
}

static int count_recent_reboots(time_t now) {
    FILE *f = fopen(REBOOT_FILE, "r");
    if (!f)
        return 0;

    int count = 0;
    time_t t;

    while (fscanf(f, "%ld", &t) == 1) {
        if (now - t <= SECONDS_24H)
            count++;
    }

    fclose(f);
    return count;
}

static int read_cpu_percent(void) {
    static uint64_t last_total = 0, last_idle = 0;

    FILE *f = fopen("/proc/stat", "r");
    if (!f)
        return -1;

    uint64_t user, nice, system, idle, iowait, irq, softirq;
    fscanf(f, "cpu %lu %lu %lu %lu %lu %lu %lu",
           &user, &nice, &system, &idle, &iowait, &irq, &softirq);
    fclose(f);

    uint64_t idle_all = idle + iowait;
    uint64_t total = user + nice + system + idle + iowait + irq + softirq;

    uint64_t delta_total = total - last_total;
    uint64_t delta_idle = idle_all - last_idle;

    last_total = total;
    last_idle = idle_all;

    if (delta_total == 0)
        return 0;

    return (int)(100 * (delta_total - delta_idle) / delta_total);
}

static int read_mem_percent(void) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f)
        return -1;

    uint64_t total = 0, available = 0;
    char key[32];
    uint64_t value;

    while (fscanf(f, "%31s %lu kB", key, &value) == 2) {
        if (strcmp(key, "MemTotal:") == 0)
            total = value;
        else if (strcmp(key, "MemAvailable:") == 0)
            available = value;
    }
    fclose(f);

    if (total == 0)
        return -1;

    return (int)(100 * (total - available) / total);
}

static int read_disk_percent(void) {
    struct statvfs vfs;
    if (statvfs("/", &vfs) != 0)
        return -1;

    uint64_t total = vfs.f_blocks * vfs.f_frsize;
    uint64_t avail = vfs.f_bavail * vfs.f_frsize;

    if (total == 0)
        return -1;

    return (int)(100 * (total - avail) / total);
}

static bool check_internet(void) {
    return system("ping -c 1 -W 1 8.8.8.8 > /dev/null 2>&1") == 0;
}

/* ---------------------------
 * Initialization
 * --------------------------- */
void state_init(void) {
    memset(&g_system, 0, sizeof(g_system));
    memset(&g_uptime, 0, sizeof(g_uptime));
    memset(&g_libre, 0, sizeof(g_libre));
    memset(&g_internet, 0, sizeof(g_internet));
    memset(&g_identity, 0, sizeof(g_identity));

    /* Sensible defaults */
    g_system.state = HEALTH_DEGRADED;
    g_libre.reachable = false;
    g_internet.reachable = false;

    /* Reboot tracking */
    time_t now = time(NULL);
    record_reboot(now);
    g_uptime.reboot_count_24h = count_recent_reboots(now);

    libre_config_load();
}

static void poll_internet_quality(void) {
    /* We use a more detailed ping to get latency and loss */
    /* -c 5 (5 packets), -i 0.2 (0.2s interval), -W 1 (1s timeout) */
    char cmd[] = "ping -c 5 -i 0.2 -W 1 8.8.8.8 2>&1";
    FILE *p = popen(cmd, "r");
    if (!p) {
        g_internet.reachable = false;
        g_internet.avg_latency_ms = 0;
        g_internet.packet_loss_percent = 100;
        return;
    }

    char buf[256];
    int packets_transmitted = 0;
    int packets_received = 0;
    float avg_latency = 0;

    while (fgets(buf, sizeof(buf), p)) {
        if (strstr(buf, "packets transmitted")) {
            sscanf(buf, "%d packets transmitted, %d received", &packets_transmitted, &packets_received);
        } else if (strstr(buf, "rtt min/avg/max/mdev")) {
            /* Example: rtt min/avg/max/mdev = 14.123/15.456/16.789/1.234 ms */
            char *slash = strchr(buf, '=');
            if (slash) {
                sscanf(slash + 1, "%*f/%f/%*f/%*f", &avg_latency);
            }
        }
    }
    pclose(p);

    if (packets_transmitted > 0) {
        g_internet.reachable = (packets_received > 0);
        g_internet.packet_loss_percent = 100 * (packets_transmitted - packets_received) / packets_transmitted;
        g_internet.avg_latency_ms = (int)avg_latency;
    } else {
        g_internet.reachable = false;
        g_internet.packet_loss_percent = 100;
        g_internet.avg_latency_ms = 0;
    }
}

/* ---------------------------
 * Update Dispatcher
 * --------------------------- */
void state_update(uint32_t dt_ms) {
    identity_timer += dt_ms;
    if (identity_timer >= IDENTITY_UPDATE_MS) {
        identity_timer = 0;

        /* Hostname */
        if (gethostname(g_identity.hostname, sizeof(g_identity.hostname)) != 0) {
            strncpy(g_identity.hostname, "UNKNOWN", sizeof(g_identity.hostname));
        }
        g_identity.hostname[sizeof(g_identity.hostname) - 1] = '\0';

        /* Primary IP */
        if (get_primary_ipv4(g_identity.ip, sizeof(g_identity.ip)) != 0) {
            strncpy(g_identity.ip, "0.0.0.0", sizeof(g_identity.ip));
        }
        g_identity.ip[sizeof(g_identity.ip) - 1] = '\0';

        /* Interface name (optional but useful) */
        strncpy(g_identity.iface, "eth0", sizeof(g_identity.iface));
        g_identity.iface[sizeof(g_identity.iface) - 1] = '\0';
    }

    uptime_timer += dt_ms;
    if (uptime_timer >= UPTIME_UPDATE_MS) {
        uptime_timer = 0;

        struct timespec ts;
        if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
            g_uptime.uptime_seconds = ts.tv_sec;
        }

        /* Recalculate reboot count occasionally */
        static uint32_t reboot_timer = 0;
        reboot_timer += UPTIME_UPDATE_MS;
        if (reboot_timer >= 60000) {  // every 60s
            reboot_timer = 0;
            time_t now = time(NULL);
            g_uptime.reboot_count_24h = count_recent_reboots(now);
        }
    }

    health_timer += dt_ms;
    if (health_timer >= HEALTH_UPDATE_MS) {
        health_timer = 0;

        int cpu = read_cpu_percent();
        int mem = read_mem_percent();
        int disk = read_disk_percent();
        bool net = check_internet();

        g_system.cpu_percent = cpu;
        g_system.mem_percent = mem;
        g_system.disk_percent = disk;
        g_system.internet_reachable = net;

        /* Determine health state */
        if (cpu < 0 || mem < 0 || disk < 0) {
            g_system.state = HEALTH_FAILED;
        } else if (cpu > 80 || mem > 80 || disk > 90 || !net) {
            g_system.state = HEALTH_DEGRADED;
        } else {
            g_system.state = HEALTH_OK;
        }
    }

    libre_timer += dt_ms;
    if (libre_timer >= LIBRE_UPDATE_MS) {
        libre_timer = 0;
        poll_libre();
    }

    /* ---------------------------
     * Internet Quality Poller
     * --------------------------- */
    internet_timer += dt_ms;
    if (internet_timer >= INTERNET_UPDATE_MS) {
        internet_timer = 0;
        poll_internet_quality();
    }
}

/* ---------------------------
 * Accessors
 * --------------------------- */
const struct system_health *state_system(void) {
    return &g_system;
}

const struct uptime_stats *state_uptime(void) {
    return &g_uptime;
}

const struct libre_stats *state_libre(void) {
    return &g_libre;
}

const struct internet_stats *state_internet(void) {
    return &g_internet;
}

const struct identity_info *state_identity(void) {
    return &g_identity;
}
