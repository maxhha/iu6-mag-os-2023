#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <argp.h>
#include <unistd.h>
#include <netdb.h>


#include "../inc/main.h"

typedef struct arguments_s
{
    char *address;
    int port;
    int wait;
} *arguments_p;

static char doc[] = "Queen of the colony";
static char args_doc[] = "[-a ADDR] [-p PORT] [-w SECONDS]";

static struct argp_option options[] = {
    {"address", 'a', "ADDR", 0, "Broadcast address of the colony (default: 255.255.255.255)", 0},
    {"port", 'p', "PORT", 0, "Port of the colony (default: 12345)", 0},
    {"wait", 'w', "SECONDS", 0, "How long should wait for the emmets (default: 10)", 0},
    {0}};

static int parse_opt(int key, char *arg, struct argp_state *state)
{
    arguments_p args = (arguments_p)state->input;

    switch (key)
    {
    case 'a':
        if (inet_addr(arg) == -1)
        {
            return 1;
        }
        args->address = arg;
        return 0;
    case 'p':
        return sscanf(arg, "%d", &args->port) != 1;
    case 'w':
        return sscanf(arg, "%d", &args->wait) != 1;
    }
    return ARGP_ERR_UNKNOWN;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

int broadcast_queen_port(const arguments_p args, in_port_t port)
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
    colony_addr.sin_addr.s_addr = inet_addr(args->address);
    colony_addr.sin_port = htons(args->port);

    ssize_t sent_n = sendto(bcast_s, &port, sizeof(port), 0, (struct sockaddr *)&colony_addr, sizeof(colony_addr));
    if (sent_n < 0)
    {
        perror("main: sendto failed");
        rc = 1;
        goto fail_sendto;
    }

    printf("Sent %ld\n", sent_n);

fail_sendto:
fail_setsockopt:
    close(bcast_s);

    return rc;
}

int main(int argc, char **argv)
{
    int rc = 0;
    struct arguments_s args = {
        .address = "255.255.255.255",
        .port = 12345,
        .wait = 10,
    };
    rc = argp_parse(&argp, argc, argv, 0, NULL, &args);
    if (rc != 0)
    {
        return rc;
    }

    int fd_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    int enable = 1;
    if (setsockopt(fd_s, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(enable)) < 0)
    {
        perror("main: setsockopt");
        rc = 1;
        goto fail_setsockopt;
    }

    if (ioctl(fd_s, FIONBIO, &enable) < 0)
    {
        perror("main: ioctl");
        rc = 1;
        goto fail_ioctl;
    }

    struct sockaddr_in addr = {0};
    socklen_t addr_len = sizeof(addr);

    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // TODO: move to args

    if (bind(fd_s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
    {
        perror("main: bind");
        rc = 1;
        goto fail_bind;
    }

    if (getsockname(fd_s, (struct sockaddr *)&addr, &addr_len) < 0)
    {
        perror("main: getsockname");
        rc = 1;
        goto fail_getsockname;
    }

    if (listen(fd_s, 32) < 0)
    { // TODO: move listen limit to args
        perror("main: listen");
        rc = 1;
        goto fail_listen;
    }

    printf("Queen listen tcp conn on port %d\n", ntohs(addr.sin_port));

    rc = broadcast_queen_port(&args, addr.sin_port);
    if (rc != 0)
    {
        goto fail_broadcast_queen_port;
    }

    struct pollfd fds[200];
    int nfds = 0;
    memset(fds, 0, sizeof(fds));

    printf("Waiting for emmets...\n");

    for (int i = 0; i < args.wait; i++)
    {
        sleep(1);
        printf(".\n");

        while (1)
        {
            struct sockaddr_in addr = {0};
            socklen_t addr_len = sizeof(addr);
            int new_s = accept(fd_s, (struct sockaddr *)&addr, &addr_len);
            if (new_s < 0)
            {
                if (errno != EWOULDBLOCK)
                {
                    perror("main: accept");
                    goto fail_accept;
                }
                break;
            }

            char remote_host[256];
            char remote_port[256];
            int err;

            if ((err = getnameinfo((struct sockaddr *)&addr, addr_len,
                            remote_host, sizeof(remote_host),
                            remote_port, sizeof(remote_port),
                            0)) != 0)
            {
                fprintf(stderr, "main: getnameinfo failed with err=%d\n", err);
            } else {
                printf("+ Ж %s:%s\n", remote_host, remote_port);
            }

            fds[nfds].fd = new_s;
            fds[nfds].events = POLLIN;
            nfds++;
        }
    }

    if (nfds == 0) {
        fprintf(stderr, "No emmets, exit\n");
        rc = 1;
        goto fail_empty_fds;
    }

    printf("Total emmets: %d\n", nfds);

    // fds[0].fd = fd_s;
    // fds[0].events = POLLIN;

    // while (1)
    // {
    //     int timeout = args.wait * 1000;
    //     printf("Waiting for emmets...\n");
    //     if (poll(fds, nfds, timeout) < 0) {
    //         perror("main: poll");
    //         rc = 1;
    //         goto fail_poll;
    //     }
    // }

fail_empty_fds:
fail_accept:
    for (int i = 0; i < nfds; i++) {
        close(fds[i].fd);
    }

fail_broadcast_queen_port:
fail_listen:
fail_getsockname:
fail_bind:
fail_ioctl:
fail_setsockopt:
    close(fd_s);

    return rc;
}
