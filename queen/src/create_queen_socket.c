#include <stdio.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <string.h>

#include "../inc/create_queen_socket.h"


int create_queen_socket(struct sockaddr_in *queen_addr, int *queen_s) {
    int rc = 0;

    int fd_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (fd_s < 0) {
        perror("create_queen_socket: socket");
        return 1;
    }

    int enable = 1;
    if (setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
    {
        perror("create_queen_socket: setsockopt");
        rc = 1;
        goto fail_setsockopt;
    }

    if (ioctl(fd_s, FIONBIO, &enable) < 0)
    {
        perror("create_queen_socket: ioctl");
        rc = 1;
        goto fail_ioctl;
    }

    if (bind(fd_s, (struct sockaddr *) queen_addr, sizeof(struct sockaddr_in)) < 0)
    {
        perror("create_queen_socket: bind");
        rc = 1;
        goto fail_bind;
    }

    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);

    if (getsockname(fd_s, (struct sockaddr *)&addr, &addr_len) < 0)
    {
        perror("create_queen_socket: getsockname");
        rc = 1;
        goto fail_getsockname;
    }

    memcpy(queen_addr, &addr, addr_len);
    *queen_s = fd_s;

    return 0;

fail_bind:
fail_ioctl:
fail_setsockopt:
fail_getsockname:
    close(fd_s);

    return rc;
}
