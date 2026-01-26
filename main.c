#include <signal.h>
#include <time.h>
#include <string.h>

#include "fb/fb.h"
#include "stats/state.h"
#include "ui/state_icons.h"
#include "ui/render_state.h"

#include "stats/host.h"
#include "stats/ip.h"
#include "ui/render_network.h"


#define BLACK 0x0000

static volatile int running = 1;

static void handle_signal(int sig) {
    (void)sig;
    running = 0;
}

int main(void) {
    struct fb fb;
    struct state_icons icons;

    signal(SIGINT, handle_signal);
    signal(SIGTERM, handle_signal);

    if (fb_init(&fb) != 0)
        return 1;

    if (load_state_icons(&icons) != 0)
        return 1;

    struct timespec delay = {
        .tv_sec = 0,
        .tv_nsec = 500 * 1000 * 1000  // 500 ms
    };

    while (running) {
        enum system_state state = get_system_state();

        char hostname[64] = "UNKNOWN";
        char ip[32] = "0.0.0.0";

        get_hostname(hostname, sizeof(hostname));
        get_primary_ipv4(ip, sizeof(ip));
        
        fb_clear(&fb, BLACK);
        render_state(&fb, state, &icons);
        render_network_info(&fb, hostname, ip);
        fb_present(&fb);


        nanosleep(&delay, NULL);
    }

    free_state_icons(&icons);
    fb_close(&fb);
    return 0;
}
