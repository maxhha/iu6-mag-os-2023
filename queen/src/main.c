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
#include "../inc/broadcast_queen_port.h"
#include "../inc/create_queen_socket.h"
#include "../inc/queen_state.h"
#include "../inc/append_duties_from_stdin.h"

typedef struct arguments_s
{
    char *address;
    int port;
    int wait;
    int duty_max_size;
} *arguments_p;

typedef struct job_s
{
    int state;
    int emmet_s;
} *job_p;

#define JOB_WAITING 0
#define JOB_PROCESSING 1
#define JOB_RECEIVING 2

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

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = 0; // TODO: move to args

    int queen_s;
    rc = create_queen_socket(&addr, &queen_s);
    if (rc != 0)
    {
        return 1;
    }

    if (listen(queen_s, 32) < 0)
    { // TODO: move listen limit to args
        perror("main: listen");
        rc = 1;
        goto fail_listen;
    }
    fprintf(stderr, "Queen listen tcp conn on port %d\n", ntohs(addr.sin_port));

    queen_state_p state = create_queen_state(queen_s);
    if (state == NULL) {
        goto fail_create_queen_state;
    }

    if (append_duties_from_stdin(state->duties, args.duty_max_size)) {
        goto fail_append_duties_from_stdin;
    }

    rc = broadcast_queen_port(args.address, args.port, addr.sin_port);
    if (rc != 0)
    {
        goto fail_broadcast_queen_port;
    }

    fprintf(stderr, "Waiting for emmet...\n");

    while (state->duties->size > 0) {
        rc = run_queen(state, args.wait * 1000);
        if (rc != 0) {
            goto fail_run_queen;
        }
    }

    // while (state.duties_n > 0)
    // {
    //     int timeout = args.wait * 1000;
    //     int fds_n = 1 + state.emmets_n;
    //     int poll_rc = poll(state.fds, fds_n, timeout);
    //     if (poll_rc < 0)
    //     {
    //         perror("main: poll");
    //         rc = 1;
    //         goto fail_poll;
    //     }

    //     if (poll_rc == 0) {
    //         int emmet_i = 0;
    //         while (emmet_i < state.emmets_n && state.emmets[emmet_i].state == EMMET_STATE_EMPTY) emmet_i++;
    //         if (emmet_i >= state.emmets_n) {
    //             fprintf(stderr, "main: poll timeout and no emmets cause death\n");
    //             rc = 1;
    //             goto fail_poll;
    //         }
    //     }

    //     for (int i = 0; i < fds_n; i++)
    //     {
    //         if (state.fds[i].revents == 0 || state.fds[i].events == 0)
    //         {
    //             continue;
    //         }

    //         if (state.fds[i].fd == queen_s)
    //         {
    //             if (state.fds[i].revents != POLLIN)
    //             {
    //                 fprintf(stderr, "main: queen_s revents = %d\n", state.fds[i].revents);
    //                 rc = 1;
    //                 goto fail_queen_revents;
    //             }

    //             for (;;)
    //             {
    //                 struct sockaddr_in addr = {0};
    //                 socklen_t addr_len = sizeof(addr);
    //                 int new_s = accept(queen_s, (struct sockaddr *)&addr, &addr_len);
    //                 if (new_s < 0)
    //                 {
    //                     if (errno != EWOULDBLOCK)
    //                     {
    //                         perror("main: accept");
    //                         goto fail_accept;
    //                     }
    //                     break;
    //                 }

    //                 char remote_host[256] = "unknown";
    //                 char remote_port[256] = "unknown";
    //                 int err;

    //                 if ((err = getnameinfo((struct sockaddr *)&addr, addr_len,
    //                                        remote_host, sizeof(remote_host),
    //                                        remote_port, sizeof(remote_port),
    //                                        0)) != 0)
    //                 {
    //                     fprintf(stderr, "main: getnameinfo failed with err=%d\n", err);
    //                 }

    //                 if (state.emmets_n >= MAX_EMMETS) {
    //                     fprintf(stderr, "main: fail accept new connection due to MAX_EMMETS limit\n");
    //                     close(new_s);
    //                     continue;
    //                 }

    //                 int emmet_i = state.emmets_n;
    //                 state.emmets[emmet_i].state = EMMET_STATE_CHILLING;
    //                 fprintf(stderr, "+ Ж #%d %s:%s\n", emmet_i + 1, remote_host, remote_port);

    //                 int fd_i = emmet_i + 1;
    //                 state.fds[fd_i].fd = new_s;
    //                 state.fds[fd_i].events = POLLIN;
    //                 state.emmets_n += 1;
    //             }
    //             continue;
    //         }

    //         int emmet_i = i - 1;

    //         if (state.fds[i].revents & POLLERR) {
    //             fprintf(stderr, "main: revents contain POLLERR\n");
    //             fprintf(stderr, "- Ж #%d\n", emmet_i + 1);
    //             state.emmets[emmet_i].state = EMMET_STATE_EMPTY;
    //             state.fds[i].fd = 0;
    //             state.fds[i].events = 0;
    //             continue;
    //         }

    //         if (state.fds[i].revents & POLLIN) {
    //             ssize_t recv_n = recv(state.fds[i].fd, NULL, 0, MSG_PEEK);
    //             if (recv_n <= 0) {
    //                 perror("main: recv from emmet");
    //                 fprintf(stderr, "- Ж #%d\n", emmet_i + 1);
    //                 state.emmets[emmet_i].state = EMMET_STATE_EMPTY;
    //                 state.fds[i].events = 0;
    //                 continue;
    //             }

    //             fprintf(stderr, "R Ж #%d -> [%ld]\n", emmet_i + 1, recv_n);
    //         }

    //         fprintf(stderr, "main: unsupported emmet revents %d\n", state.fds[i].revents);
    //     }

    //     for (int duty_i = 0; duty_i < state.duties_n; duty_i++) {
    //         if (state.duties[duty_i].type != DUTY_TYPE_DELEGATE
    //         && state.duties[duty_i].state != DUTY_STATE_WAITING) {
    //             continue;
    //         }

    //         for (int emmet_i = 0; emmet_i < state.emmets_n; emmet_i++) {
    //             if (state.emmets[emmet_i].state != EMMET_STATE_CHILLING) {
    //                 continue;
    //             }

    //             struct { int action; int duty_i; } msg = {
    //                 .action = state.duties[duty_i].type,
    //                 .duty_i = duty_i,
    //             };

    //             ssize_t sent_n = send(state.fds[emmet_i + 1].fd, &msg, sizeof(msg), 0);
    //             if (sent_n < 0) {
    //                 // TODO: check sent size
    //                 fprintf(stderr, "main: send emmet #%d duty\n", emmet_i + 1);
    //                 state.emmets[emmet_i].state = EMMET_STATE_EMPTY;
    //             } else {
    //                 fprintf(stderr, "S Ж #%d <- duty #%d [%ld]\n", emmet_i + 1, duty_i + 1, sent_n);
    //                 state.emmets[emmet_i].processing_duty_i = duty_i;
    //                 state.emmets[emmet_i].state = EMMET_STATE_HARD_WORKING;

    //                 state.duties[duty_i].state = DUTY_STATE_PROCESSING;
    //                 break;
    //             }
    //         }

    //         if (state.duties[duty_i].state == DUTY_STATE_WAITING) {
    //             break;
    //         }
    //     }
    // }

fail_run_queen:
fail_broadcast_queen_port:
fail_append_duties_from_stdin:
    free_queen_state(state);

fail_create_queen_state:
fail_listen:
    close(queen_s);

    return rc;
}
