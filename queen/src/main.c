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
    {"duty_max_size", 'd', "NUMBER", 0, "Max amount of work for one emmet duty (default: 2048)", 0},
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
    case 'd':
        return sscanf(arg, "%d", &args->duty_max_size) != 1;
    }
    return ARGP_ERR_UNKNOWN;
}

static struct argp argp = {options, parse_opt, args_doc, doc};

typedef struct printing_list_s {
    int pos;
    int size;
    int *value;
    struct printing_list_s* next;
} *printing_list_p;

printing_list_p create_printing_list(int size, int *buf) {
    printing_list_p l = (printing_list_p) malloc(sizeof(struct printing_list_s));
    if (l == NULL) {
        fprintf(stderr, "create_printing_list: malloc\n");
        return NULL;
    }
    l->pos = 0;
    l->size = size;
    l->value = buf;
    l->next = NULL;
    return l;
}

void free_printing_list(printing_list_p l) {
    while (l != NULL) {
        printing_list_p n = l->next;
        free(l);
        l = n;
    }
}

void print_merged_result(vec_duty_p_p duties) {
    fprintf(stderr, "Printing result for %d duties...\n", duties->size);
    if (duties->size > 10) {
        fprintf(stderr, "HINT: increase duty_max_size to make it faster (current: %d)\n", duties->data[0]->size);
    }

    // TODO: use heap
    int n = duties->size;
    size_t total_size = 0;
    size_t printed_n = 0;

    printing_list_p head = NULL;
    printing_list_p *p = &head;

    for (int i = 0; i < n; i++) {
        duty_p d = duties->data[i];
        *p = create_printing_list(d->size, d->result);
        if (*p == NULL) {
            goto fail_create_printing_list;
        }
        total_size += d->size;
        p = &((*p)->next);
    }

    while(head != NULL)
    {
        printing_list_p p_min = head;
        printing_list_p p_min_prev = NULL;

        for (printing_list_p p_prev = head, p = head->next; p != NULL; p_prev = p, p = p->next) {
            if (*p->value < *p_min->value) {
                p_min_prev = p_prev;
                p_min = p;
            }
        }

        printf("%d\n", *p_min->value);
        p_min->value++;
        p_min->pos++;

        if (p_min->pos >= p_min->size) {
            printing_list_p p = p_min->next;
            p_min->next = NULL;
            free_printing_list(p_min);

            if (p_min_prev == NULL) {
                head = p;
            } else {
                p_min_prev->next = p;
            }
        }

        printed_n += 1;
        if (printed_n % (total_size / 10) == 0) {
            fprintf(stderr, "Progress: %.0f%%\n", (float) printed_n / total_size * 100);
        }
    }

    fprintf(stderr, "Done\n");
fail_create_printing_list:
    free_printing_list(head);
}

int main(int argc, char **argv)
{
    int rc = 0;
    struct arguments_s args = {
        .address = "255.255.255.255",
        .port = 12345,
        .wait = 10,
        .duty_max_size = 2048,
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

    while (state->finished_duties_n < state->duties->size) {
        fprintf(stderr, "\nQ Progress: %.0lf%%\n\n", (float) state->finished_duties_n * 100 / state->duties->size);

        rc = run_queen(state, args.wait * 1000);
        if (rc != 0) {
            goto fail_run_queen;
        }
    }
    print_merged_result(state->duties);

fail_run_queen:
fail_broadcast_queen_port:
fail_append_duties_from_stdin:
    free_queen_state(state);

fail_create_queen_state:
fail_listen:
    close(queen_s);

    return rc;
}
