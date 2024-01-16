#include <stdlib.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <errno.h>

#include "../inc/queen_state.h"

#define T duty_p
#define FREE_T() free_duty
#include "vec.tmpl.c"
#undef FREE_T
#undef T

queen_state_p create_queen_state(int queen_socket)
{
    queen_state_p s = (queen_state_p)malloc(sizeof(struct queen_state_s));
    if (s == NULL)
    {
        perror("create_queen_state: malloc");
        return NULL;
    }

    s->queen_s = queen_socket;
    s->emmets_n = 0;

    memset(&s->fds, 0, sizeof(s->fds));
    s->fds[0].fd = queen_socket;
    s->fds[0].events = POLLIN;

    s->duties = create_vec_duty_p();
    if (s->duties == NULL)
    {
        goto fail_create_vec_duty;
    }
    s->finished_duties_n = 0;

    return s;

fail_create_vec_duty:
    free(s);

    return NULL;
}

int process_queen_socket_events(queen_state_p s, struct pollfd *fd)
{
    if (fd->revents != POLLIN)
    {
        fprintf(stderr, "process_queen_socket_events: revents = %d\n", fd->revents);
        return 1;
    }

    for (;;)
    {
        struct sockaddr_in addr = {0};
        socklen_t addr_len = sizeof(addr);
        int new_s = accept(fd->fd, (struct sockaddr *)&addr, &addr_len);
        if (new_s < 0)
        {
            if (errno != EWOULDBLOCK)
            {
                perror("process_queen_socket_events: accept");
                return 1;
            }
            break;
        }

        char remote_host[256] = "unknown";
        char remote_port[256] = "unknown";
        int err;

        if ((err = getnameinfo((struct sockaddr *)&addr, addr_len,
                               remote_host, sizeof(remote_host),
                               remote_port, sizeof(remote_port),
                               0)) != 0)
        {
            fprintf(stderr, "process_queen_socket_events: getnameinfo failed with err=%d\n", err);
        }

        if (s->emmets_n >= MAX_EMMETS)
        {
            fprintf(stderr, "process_queen_socket_events: fail accept new connection due to MAX_EMMETS limit\n");
            close(new_s);
            continue;
        }

        int emmet_i = s->emmets_n;
        s->emmets[emmet_i].state = EMMET_STATE_CHILLING;
        s->emmets[emmet_i].result_buf = NULL;
        s->emmets[emmet_i].received_result_n = 0;

        fprintf(stderr, "+ Ж #%d %s:%s\n", emmet_i + 1, remote_host, remote_port);

        int fd_i = emmet_i + 1;
        s->fds[fd_i].fd = new_s;
        s->fds[fd_i].events = POLLIN;
        s->emmets_n += 1;
    }

    return 0;
}

void drop_emmet(queen_state_p s, int emmet_i)
{
    fprintf(stderr, "- Ж #%d\n", emmet_i + 1);

    int fd_i = emmet_i + 1;
    if (s->emmets[emmet_i].state == EMMET_STATE_HARD_WORKING) {
        s->duties->data[s->emmets[emmet_i].processing_duty_i]->state = DUTY_STATE_WAITING;
    }

    s->emmets[emmet_i].state = EMMET_STATE_EMPTY;

    if (s->emmets[emmet_i].result_buf != NULL) {
        free(s->emmets[emmet_i].result_buf);
    }

    s->fds[fd_i].fd = 0;
    s->fds[fd_i].events = 0;

    while (s->emmets_n > 0 && s->emmets[s->emmets_n - 1].state == EMMET_STATE_EMPTY) {
        s->emmets_n--;
    }
}

int process_emmet_socket_events(queen_state_p s, struct pollfd *fd, int fd_i)
{
    int emmet_i = fd_i - 1;

    if (fd->revents & POLLERR)
    {
        fprintf(stderr, "process_emmet_socket_events: revents contain POLLERR\n");

        drop_emmet(s, emmet_i);

        return 0;
    }

    if (fd->revents & POLLIN)
    {
        emmet_p emmet = &s->emmets[emmet_i];
        if (emmet->state != EMMET_STATE_HARD_WORKING && emmet->state != EMMET_STATE_CARRYING_RESULT) {
            fprintf(stderr, "process_emmet_socket_events: emmet is in wrong state 0x%x\n", emmet->state);

            drop_emmet(s, emmet_i);

            return 0;
        }

        int duty_i = s->emmets[emmet_i].processing_duty_i;
        duty_p d = s->duties->data[duty_i];
        size_t duty_size_bytes = sizeof(int) * d->size;

        if (emmet->state == EMMET_STATE_HARD_WORKING) {
            emmet->state = EMMET_STATE_CARRYING_RESULT;
            emmet->received_result_n = 0;
            emmet->result_buf = (char *) malloc(duty_size_bytes);
        }

        ssize_t recv_n = recv(fd->fd, ((char *) emmet->result_buf) + emmet->received_result_n, duty_size_bytes - emmet->received_result_n, 0);
        if (recv_n <= 0)
        {
            perror("process_emmet_socket_events: recv from emmet");
            drop_emmet(s, emmet_i);

            return 0;
        }

        emmet->received_result_n += recv_n;

        fprintf(stderr, "R Ж #%d -[%3.0f%%]-> duty #%d\n", emmet_i + 1, (float) emmet->received_result_n / duty_size_bytes * 100, duty_i + 1);

        if (emmet->received_result_n == duty_size_bytes) {
            d->result = (int *) emmet->result_buf;
            free(d->input);
            d->input = NULL;
            d->state = DUTY_STATE_FINISHED;

            emmet->result_buf = NULL;
            emmet->state = EMMET_STATE_CHILLING;

            s->finished_duties_n++;
        }
        else if (emmet->received_result_n > duty_size_bytes) {
            fprintf(stderr, "process_emmet_socket_events: received_result_n is greater than duty_size_bytes: %ld > %ld\n", emmet->received_result_n, duty_size_bytes);
            drop_emmet(s, emmet_i);

            return 0;
        }

        return 0;
    }

    fprintf(stderr, "process_emmet_socket_events: unsupported emmet revents %d\n", fd->revents);

    return 0;
}

int process_duties(queen_state_p s)
{
    for (int duty_i = 0; duty_i < s->duties->size; duty_i++)
    {
        // fprintf(stderr, "check duty #%d\n", duty_i + 1);
        duty_p d = s->duties->data[duty_i];
        if (d->state != DUTY_STATE_WAITING)
        {
            continue;
        }

        for (int emmet_i = 0; emmet_i < s->emmets_n; emmet_i++)
        {
            // fprintf(stderr, "check emmet #%d\n", emmet_i + 1);
            emmet_p emmet = &s->emmets[emmet_i];

            if (emmet->state != EMMET_STATE_CHILLING)
            {
                continue;
            }

            // fprintf(stderr, "its chilling!\n");

            int fd_i = emmet_i + 1;

            ssize_t sent_n = send(s->fds[fd_i].fd, &d->size, sizeof(int), 0);
            if (sent_n < 0 || sent_n != sizeof(int))
            {
                fprintf(stderr, "process_duties: send emmet #%d duty size of [%ld] but sent [%ld]\n", emmet_i + 1, sizeof(int), sent_n);
                drop_emmet(s, emmet_i);
                continue;
            }

            sent_n = send(s->fds[fd_i].fd, d->input, sizeof(int) * d->size, 0);
            if (sent_n < 0 || sent_n != sizeof(int) * d->size)
            {
                fprintf(stderr, "process_duties: send emmet #%d duty [%ld] but sent [%ld]\n", emmet_i + 1, sizeof(int) * d->size, sent_n);
                drop_emmet(s, emmet_i);
                continue;
            }

            fprintf(stderr, "S Ж #%d <- duty #%d [%d]\n", emmet_i + 1, duty_i + 1, d->size);
            emmet->processing_duty_i = duty_i;
            emmet->state = EMMET_STATE_HARD_WORKING;

            d->state = DUTY_STATE_PROCESSING;
            break;
        }

        if (d->state == DUTY_STATE_WAITING)
        {
            break;
        }
    }

    // fprintf(stderr, "process_duties: finished\n");

    return 0;
}

int run_queen(queen_state_p s, int timeout)
{
    // fprintf(stderr, "run_queen: start\n");

    int fds_n = 1 + s->emmets_n;
    int poll_rc = poll(s->fds, fds_n, timeout);
    if (poll_rc < 0)
    {
        perror("run_queen: poll");
        return 1;
    }

    if (poll_rc == 0)
    {
        int emmet_i = 0;
        while (emmet_i < s->emmets_n && s->emmets[emmet_i].state == EMMET_STATE_EMPTY)
            emmet_i++;
        if (emmet_i >= s->emmets_n)
        {
            fprintf(stderr, "run_queen: poll timeout and no emmets cause death\n");
            return 1;
        }
    }

    int rc = 0;
    for (int i = 0; rc == 0 && i < fds_n; i++)
    {
        if (s->fds[i].revents == 0 || s->fds[i].events == 0)
        {
            continue;
        }

        if (s->fds[i].fd == s->queen_s)
        {
            rc = process_queen_socket_events(s, &s->fds[i]);
        }
        else
        {
            rc = process_emmet_socket_events(s, &s->fds[i], i);
        }
    }

    if (rc != 0)
    {
        return rc;
    }

    rc = process_duties(s);

    return rc;
}

duty_p create_duty_from_input(int *input, int size) {
    duty_p d = (duty_p) malloc(sizeof(struct duty_s));
    if (d == NULL) {
        fprintf(stderr, "create_duty_from_input: failed to malloc duty\n");
        return NULL;
    }
    d->state = DUTY_STATE_WAITING;
    d->size = size;
    d->input = input;
    d->result = NULL;
    return d;
}

void free_duty(duty_p d)
{
    if (d->input != NULL)
    {
        free(d->input);
    }
    if (d->result != NULL)
    {
        free(d->result);
    }

    free(d);
}

void free_queen_state(queen_state_p s)
{
    for (int i = 0; i < s->emmets_n; i++)
    {
        close(s->fds[i + 1].fd);
    }

    free_vec_duty_p(s->duties);
    free(s);
}
