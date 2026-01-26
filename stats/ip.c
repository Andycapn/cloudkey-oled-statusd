#include "ip.h"
#include <stdio.h>
#include <string.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <sys/socket.h>
#include <linux/rtnetlink.h>
#include <unistd.h>

int get_primary_ipv4(char *buf, int buflen) {
    int sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0)
        return -1;

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(53),
    };

    /* Doesn't actually send packets */
    inet_pton(AF_INET, "8.8.8.8", &addr.sin_addr);
    connect(sock, (struct sockaddr *)&addr, sizeof(addr));

    struct sockaddr_in local;
    socklen_t len = sizeof(local);
    if (getsockname(sock, (struct sockaddr *)&local, &len) != 0) {
        close(sock);
        return -1;
    }

    const char *ip = inet_ntoa(local.sin_addr);
    strncpy(buf, ip, buflen);
    buf[buflen - 1] = '\0';

    close(sock);
    return 0;
}
