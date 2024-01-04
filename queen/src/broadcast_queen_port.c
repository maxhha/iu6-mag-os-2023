#include <stdio.h>
#include <unistd.h>

#include "../inc/broadcast_queen_port.h"


int broadcast_queen_port(const char *bcast_address, int bcast_host_port, in_port_t queen_port)
{
    int rc = 0;
    int bcast_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bcast_s < 0)
    {
        fprintf(stderr, "broadcast_queen_port: socket: fail to create bcast_s\n");
        return 1;
    }

    int broadcast_enable = 1;
    if (setsockopt(bcast_s, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)))
    {
        perror("broadcast_queen_port: setsockopt");
        rc = 1;
        goto fail_setsockopt;
    }

    struct sockaddr_in colony_addr = {0};
    colony_addr.sin_family = AF_INET;
    colony_addr.sin_addr.s_addr = inet_addr(bcast_address);
    colony_addr.sin_port = htons(bcast_host_port);

    ssize_t sent_n = sendto(bcast_s, &queen_port, sizeof(queen_port), 0, (struct sockaddr *)&colony_addr, sizeof(colony_addr));
    if (sent_n < 0)
    {
        perror("broadcast_queen_port: sendto failed");
        rc = 1;
        goto fail_sendto;
    }

fail_sendto:
fail_setsockopt:
    close(bcast_s);

    return rc;
}
