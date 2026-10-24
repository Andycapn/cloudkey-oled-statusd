#include "state.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <stdlib.h>
#include <inttypes.h>

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

#define IDENTITY_UPDATE_MS 5000u
#define UPTIME_UPDATE_MS 1000u
#define HEALTH_UPDATE_MS 2000u
#define LIBRE_UPDATE_MS 15000u
#define INTERNET_UPDATE_MS 5000u
#define REBOOT_CHECK_INTERVAL_MS 60000u

struct state_scheduler {
    uint32_t identity_ms;
    uint32_t uptime_ms;
    uint32_t health_ms;
    uint32_t libre_ms;
    uint32_t internet_ms;
    uint32_t reboot_ms;
};

static struct state_scheduler g_sched;

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

/* Runs argv[0] with the given arguments directly via execvp (no shell),
 * so values like the LibreNMS token/URL/device id can never be interpreted
 * as shell syntax. Returns a FILE* for reading the child's stdout. */
static FILE *exec_argv(char *const argv[], pid_t *out_pid) {
    int pipefd[2];
    if (pipe(pipefd) != 0)
        return NULL;

    pid_t pid = fork();
    if (pid < 0) {
        close(pipefd[0]);
        close(pipefd[1]);
        return NULL;
    }

    if (pid == 0) {
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO);
        close(pipefd[1]);
        int devnull = open("/dev/null", O_WRONLY);
        if (devnull >= 0) {
            dup2(devnull, STDERR_FILENO);
            close(devnull);
        }
        execvp(argv[0], argv);
        _exit(127);
    }

    close(pipefd[1]);
    FILE *f = fdopen(pipefd[0], "r");
    if (!f) {
        close(pipefd[0]);
        int status;
        waitpid(pid, &status, 0);
        return NULL;
    }

    *out_pid = pid;
    return f;
}

static int exec_close(FILE *f, pid_t pid) {
    fclose(f);
    int status;
    waitpid(pid, &status, 0);
    return WIFEXITED(status) ? WEXITSTATUS(status) : -1;
}

static const char *skip_json_space(const char *value) {
    while (*value == ' ' || *value == '\t' || *value == '\r' || *value == '\n') {
        value++;
    }
    return value;
}

static bool is_json_value_end(char value) {
    return value == '\0' || value == ',' || value == '}' ||
           value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

static bool json_string_field(const char *json, const char *field,
                              char *output, size_t capacity) {
    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", field);
    const char *cursor = strstr(json, key);
    if (!cursor) {
        return false;
    }

    cursor = skip_json_space(cursor + strlen(key));
    if (*cursor++ != ':') {
        return false;
    }
    cursor = skip_json_space(cursor);
    if (*cursor++ != '"') {
        return false;
    }

    size_t length = 0;
    while (*cursor && *cursor != '"') {
        char value = *cursor++;
        if (value == '\\' && *cursor) {
            value = *cursor++;
            if (value == 'n') value = '\n';
            else if (value == 'r') value = '\r';
            else if (value == 't') value = '\t';
        }
        if (length + 1 >= capacity) {
            return false;
        }
        output[length++] = value;
    }
    if (*cursor != '"') {
        return false;
    }
    output[length] = '\0';
    return true;
}

static bool json_number_field(const char *json, const char *field, double *output) {
    char key[64];
    snprintf(key, sizeof(key), "\"%s\"", field);
    const char *cursor = strstr(json, key);
    if (!cursor) {
        return false;
    }

    cursor = skip_json_space(cursor + strlen(key));
    if (*cursor++ != ':') {
        return false;
    }
    cursor = skip_json_space(cursor);
    bool quoted = *cursor == '"';
    if (quoted) {
        cursor++;
    }

    char *end;
    double value = strtod(cursor, &end);
    if (end == cursor || value < 0 || (quoted && *end != '"') ||
        (!quoted && !is_json_value_end(*end))) {
        return false;
    }
    *output = value;
    return true;
}

static bool encode_path_segment(const char *value, char *output, size_t capacity) {
    static const char hex[] = "0123456789ABCDEF";
    size_t length = 0;
    for (const unsigned char *cursor = (const unsigned char *)value; *cursor; cursor++) {
        bool safe = (*cursor >= 'a' && *cursor <= 'z') ||
                    (*cursor >= 'A' && *cursor <= 'Z') ||
                    (*cursor >= '0' && *cursor <= '9') ||
                    *cursor == '-' || *cursor == '.' || *cursor == '_' || *cursor == '~';
        size_t needed = safe ? 1 : 3;
        if (length + needed >= capacity) {
            return false;
        }
        if (safe) {
            output[length++] = (char)*cursor;
        } else {
            output[length++] = '%';
            output[length++] = hex[*cursor >> 4];
            output[length++] = hex[*cursor & 0x0F];
        }
    }
    output[length] = '\0';
    return true;
}

static bool core_port_selected(const char *interface) {
    const char *ports = libre_core_ports();
    if (!ports[0]) {
        return true;
    }

    size_t interface_length = strlen(interface);
    while (*ports) {
        while (*ports == ',' || *ports == ' ' || *ports == '\t') {
            ports++;
        }
        const char *start = ports;
        while (*ports && *ports != ',') {
            ports++;
        }
        const char *end = ports;
        while (end > start && (end[-1] == ' ' || end[-1] == '\t')) {
            end--;
        }
        if ((size_t)(end - start) == interface_length &&
            strncmp(start, interface, interface_length) == 0) {
            return true;
        }
        if (*ports == ',') {
            ports++;
        }
    }
    return false;
}

static void count_device_statuses(const char *json, int *up, int *down, int *warning) {
    const char *cursor = json;
    while ((cursor = strstr(cursor, "\"status\"")) != NULL) {
        const char *value = skip_json_space(cursor + sizeof("\"status\"") - 1);
        if (*value != ':') {
            cursor += sizeof("\"status\"") - 1;
            continue;
        }
        value = skip_json_space(value + 1);

        if (*value == '"') {
            const char *text = ++value;
            while (*value && *value != '"') {
                value++;
            }
            size_t length = (size_t)(value - text);
            if (length == 2 && strncmp(text, "up", length) == 0) {
                (*up)++;
            } else if (length == 4 && strncmp(text, "down", length) == 0) {
                (*down)++;
            } else if (length == 7 && strncmp(text, "warning", length) == 0) {
                (*warning)++;
            } else if (length == 1 && *text == '1') {
                (*up)++;
            } else if (length == 1 && *text == '0') {
                (*down)++;
            }
            cursor = value;
        } else if (strncmp(value, "true", 4) == 0 && is_json_value_end(value[4])) {
            (*up)++;
            cursor = value + 4;
        } else if (strncmp(value, "false", 5) == 0 && is_json_value_end(value[5])) {
            (*down)++;
            cursor = value + 5;
        } else if ((*value == '0' || *value == '1') && is_json_value_end(value[1])) {
            if (*value == '1') {
                (*up)++;
            } else {
                (*down)++;
            }
            cursor = value + 1;
        } else {
            cursor = value;
        }
    }
}

static void poll_libre(void) {
    g_libre.core_traffic_available = false;
    g_libre.configured = libre_url()[0] && libre_token()[0];
    g_libre.checked = true;
    if (!g_libre.configured) {
        g_libre.reachable = false;
        return;
    }

    char auth_header[160];
    snprintf(auth_header, sizeof(auth_header), "X-Auth-Token: %s", libre_token());

    char devices_url[300];
    snprintf(devices_url, sizeof(devices_url), "%s/devices", libre_url());

    char *argv[] = {
        "curl", "-sf", "--max-time", "5",
        "-H", auth_header,
        devices_url,
        NULL
    };

    pid_t pid;
    FILE *p = exec_argv(argv, &pid);
    if (!p) {
        g_libre.reachable = false;
        return;
    }

    char buf[8192];
    int down = 0, warn = 0;
    int ok = 0;

    while (fgets(buf, sizeof(buf), p)) {
        count_device_statuses(buf, &ok, &down, &warn);
    }

    int rc = exec_close(p, pid);
    if (rc != 0) {
        g_libre.reachable = false;
        return;
    }

    g_libre.reachable = true;
    g_libre.devices_up = ok;
    g_libre.devices_down = down;
    g_libre.devices_warning = warn;

    const char *core = libre_core_device();
    if (!core || !core[0]) {
        return;
    }

    char encoded_core[192];
    if (!encode_path_segment(core, encoded_core, sizeof(encoded_core))) {
        return;
    }

    char ports_url[512];
    snprintf(ports_url, sizeof(ports_url), "%s/devices/%s/ports", libre_url(), encoded_core);
    char *ports_argv[] = { "curl", "-sf", "--max-time", "5", "-H", auth_header, ports_url, NULL };
    pid_t ports_pid;
    FILE *ports_response = exec_argv(ports_argv, &ports_pid);
    if (!ports_response) {
        return;
    }

    char interfaces[128][64];
    int interface_count = 0;
    while (fgets(buf, sizeof(buf), ports_response)) {
        char interface[64];
        if (json_string_field(buf, "ifName", interface, sizeof(interface)) &&
            interface[0] && core_port_selected(interface) &&
            interface_count < (int)(sizeof(interfaces) / sizeof(interfaces[0]))) {
            snprintf(interfaces[interface_count++], sizeof(interfaces[0]), "%s", interface);
        }
    }
    if (exec_close(ports_response, ports_pid) != 0) {
        return;
    }

    double total_in = 0.0, total_out = 0.0;
    int ports_with_rates = 0;
    for (int index = 0; index < interface_count; index++) {
        char encoded_interface[192];
        if (!encode_path_segment(interfaces[index], encoded_interface, sizeof(encoded_interface))) {
            continue;
        }

        char port_url[768];
        snprintf(port_url, sizeof(port_url), "%s/devices/%s/ports/%s",
                 libre_url(), encoded_core, encoded_interface);
        char *port_argv[] = { "curl", "-sf", "--max-time", "5", "-H", auth_header, port_url, NULL };
        pid_t port_pid;
        FILE *port_response = exec_argv(port_argv, &port_pid);
        if (!port_response) {
            continue;
        }

        double in_rate = 0.0, out_rate = 0.0;
        bool has_in = false, has_out = false;
        while (fgets(buf, sizeof(buf), port_response)) {
            if (!has_in) has_in = json_number_field(buf, "ifInOctets_rate", &in_rate);
            if (!has_out) has_out = json_number_field(buf, "ifOutOctets_rate", &out_rate);
        }
        int port_status = exec_close(port_response, port_pid);
        if (port_status == 0 && has_in && has_out) {
            total_in += in_rate;
            total_out += out_rate;
            ports_with_rates++;
        }
    }

    if (ports_with_rates > 0) {
        g_libre.traffic_in = (uint64_t)(total_in * 8.0);
        g_libre.traffic_out = (uint64_t)(total_out * 8.0);
        g_libre.core_traffic_available = true;

        for (int i = 0; i < 59; i++) {
            g_libre.traffic_history_in[i] = g_libre.traffic_history_in[i + 1];
            g_libre.traffic_history_out[i] = g_libre.traffic_history_out[i + 1];
        }
        g_libre.traffic_history_in[59] = g_libre.traffic_in;
        g_libre.traffic_history_out[59] = g_libre.traffic_out;
    }
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
    fscanf(f, "cpu %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64 " %" SCNu64,
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

    while (fscanf(f, "%31s %" SCNu64 " kB", key, &value) == 2) {
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
    memset(&g_sched, 0, sizeof(g_sched));

    /* Sensible defaults */
    g_system.state = HEALTH_DEGRADED;
    g_libre.reachable = false;
    g_internet.reachable = false;

    /* Reboot tracking */
    time_t now = time(NULL);
    record_reboot(now);
    g_uptime.reboot_count_24h = count_recent_reboots(now);

    libre_config_load();
    g_libre.configured = libre_url()[0] && libre_token()[0];
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
    if (dt_ms == 0) {
        return;
    }

    g_sched.identity_ms += dt_ms;
    while (g_sched.identity_ms >= IDENTITY_UPDATE_MS) {
        g_sched.identity_ms -= IDENTITY_UPDATE_MS;

        if (gethostname(g_identity.hostname, sizeof(g_identity.hostname)) != 0) {
            strncpy(g_identity.hostname, "UNKNOWN", sizeof(g_identity.hostname));
        }
        g_identity.hostname[sizeof(g_identity.hostname) - 1] = '\0';

        if (get_primary_ipv4(g_identity.ip, sizeof(g_identity.ip)) != 0) {
            strncpy(g_identity.ip, "0.0.0.0", sizeof(g_identity.ip));
        }
        g_identity.ip[sizeof(g_identity.ip) - 1] = '\0';

        strncpy(g_identity.iface, "eth0", sizeof(g_identity.iface));
        g_identity.iface[sizeof(g_identity.iface) - 1] = '\0';
    }

    g_sched.uptime_ms += dt_ms;
    while (g_sched.uptime_ms >= UPTIME_UPDATE_MS) {
        g_sched.uptime_ms -= UPTIME_UPDATE_MS;

        struct timespec ts;
#if defined(CLOCK_BOOTTIME)
        if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
            g_uptime.uptime_seconds = ts.tv_sec;
        }
#elif defined(CLOCK_MONOTONIC)
        if (clock_gettime(CLOCK_MONOTONIC, &ts) == 0) {
            g_uptime.uptime_seconds = ts.tv_sec;
        }
#else
        if (clock_gettime(CLOCK_REALTIME, &ts) == 0) {
            g_uptime.uptime_seconds = ts.tv_sec;
        }
#endif
    }

    g_sched.reboot_ms += dt_ms;
    while (g_sched.reboot_ms >= REBOOT_CHECK_INTERVAL_MS) {
        g_sched.reboot_ms -= REBOOT_CHECK_INTERVAL_MS;
        time_t now = time(NULL);
        g_uptime.reboot_count_24h = count_recent_reboots(now);
    }

    g_sched.health_ms += dt_ms;
    while (g_sched.health_ms >= HEALTH_UPDATE_MS) {
        g_sched.health_ms -= HEALTH_UPDATE_MS;

        int cpu = read_cpu_percent();
        int mem = read_mem_percent();
        int disk = read_disk_percent();
        bool net = check_internet();

        g_system.cpu_percent = cpu;
        g_system.mem_percent = mem;
        g_system.disk_percent = disk;
        g_system.internet_reachable = net;

        if (cpu < 0 || mem < 0 || disk < 0) {
            g_system.state = HEALTH_FAILED;
        } else if (cpu > 80 || mem > 80 || disk > 90 || !net) {
            g_system.state = HEALTH_DEGRADED;
        } else {
            g_system.state = HEALTH_OK;
        }
    }

    g_sched.libre_ms += dt_ms;
    while (g_sched.libre_ms >= LIBRE_UPDATE_MS) {
        g_sched.libre_ms -= LIBRE_UPDATE_MS;
        poll_libre();
    }

    g_sched.internet_ms += dt_ms;
    while (g_sched.internet_ms >= INTERNET_UPDATE_MS) {
        g_sched.internet_ms -= INTERNET_UPDATE_MS;
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
