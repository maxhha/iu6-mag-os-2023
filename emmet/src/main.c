#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include <sys/socket.h>
#include <unistd.h>

#include "../inc/main.h"
#include "../inc/receive_queen_addr.h"

typedef struct arguments_s {
    int port;
    int wait;
} *arguments_p;

static char doc[] = "Worker of the colony";
static char args_doc[] = "[-p PORT] [-w SECONDS]";

static struct argp_option options[] = {
    {"port", 'p', "PORT", 0, "Port to listen for the queen (default: 12345)", 0},
    {"wait", 'w', "SECONDS", 0, "How long should wait for the queen (default: 30)", 0},
    {0}
};

static int parse_opt(int key, char *arg, struct argp_state *state) {
    arguments_p args = (arguments_p) state->input;

    switch (key)
    {
    case 'p':
        return sscanf(arg, "%d", &args->port) != 1;
    case 'w':
        return sscanf(arg, "%d", &args->wait) != 1;
    }
    return ARGP_ERR_UNKNOWN;
}

static struct argp argp = { options, parse_opt, args_doc, doc };

int compare_int(const int *a, const int *b) {
    return *a - *b;
}

int main(int argc, char **argv) {
    int rc = 0;
    struct arguments_s args = {
        .port =12345,
        .wait = 30,
    };

    rc = argp_parse(&argp, argc, argv, 0, NULL, &args);
    if (rc != 0) {
        goto fail_argp_parse;
    }

    struct sockaddr_in queen_addr;
    rc = receive_queen_addr(args.wait, args.port, &queen_addr);
    if (rc != 0) {
        goto fail_receive_queen_addr;
    }

    int emmet_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (emmet_s < 0) {
        perror("main: socket");
        rc = 1;
        goto fail_socket;
    }

    if (connect(emmet_s, (struct sockaddr *) &queen_addr, sizeof(queen_addr)) < 0) {
        perror("main: connect");
        rc = 1;
        goto fail_connect;
    }

    for(;;)
    {
        int duty_size;
        ssize_t recv_n = recv(emmet_s, &duty_size, sizeof(int), 0);
        if (recv_n < 0) {
            perror("main: recv duty_size");
            rc = 1;
            goto fail_recv;
        }

        if (recv_n != sizeof(int)) {
            fprintf(stderr, "main: duty_size recv_n not sizeof int %ld != %ld\n", recv_n, sizeof(int));
            rc = 1;
            goto fail_recv;
        }

        int *buf = (int *) malloc(duty_size * sizeof(int));
        recv_n = recv(emmet_s, buf, duty_size * sizeof(int), 0);
        if (recv_n < 0) {
            perror("main: recv input");
            free(buf);
            rc = 1;
            goto fail_recv;
        }

        if (duty_size * sizeof(int) != recv_n) {
            fprintf(stderr, "main: duty_size not match to received data size: %ld != %ld\n", duty_size * sizeof(int), recv_n);
            rc = 1;
            free(buf);
            goto fail_recv;
        }

        fprintf(stderr, "R [%d]\n", duty_size);

        // TODO: here must be parallel merge sort
        qsort(buf, duty_size, sizeof(int), (int(*) (const void *, const void *)) compare_int);

        fprintf(stderr, "F [%d]\n", duty_size);

        ssize_t sent_n = send(emmet_s, buf, sizeof(int) * duty_size, 0);
        if (sent_n < 0) {
            perror("main: send result");
            free(buf);
            rc = 1;
            goto fail_recv;
        }

        if (sent_n != sizeof(int) * duty_size) {
            fprintf(stderr, "main: sent_n mismatch %ld != %ld\n", sent_n, sizeof(int) * duty_size);
            free(buf);
            rc = 1;
            goto fail_recv;
        }
        else {
            fprintf(stderr, "S [%d]\n", duty_size);
        }

        free(buf);
    }

fail_recv:
fail_connect:
    close(emmet_s);

fail_socket:
fail_receive_queen_addr:
fail_argp_parse:
    return rc;
}
