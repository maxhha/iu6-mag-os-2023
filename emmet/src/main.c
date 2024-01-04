#include <stdlib.h>
#include <stdio.h>
#include <argp.h>

#include <sys/socket.h>
#include <unistd.h>

#include "../inc/main.h"

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

    int queen_s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (queen_s < 0) {
        perror("main: socket");
        rc = 1;
        goto fail_socket;
    }

    if (connect(queen_s, (struct sockaddr *) &queen_addr, sizeof(queen_addr)) < 0) {
        perror("main: connect");
        rc = 1;
        goto fail_connect;
    }

    /*
        Emmet creates tcp connection to queen.
        Queen gathers available emmets for work.
        Queen splits and sends data for work to emmets.
        Queen pings emmets. If someone failed
          ж  ж  ж  ж  ж  .  ?
        // print ж for every working emmet
        // print ? for every disappeared worker
        // print . when worker is free
        // when data returned
    */

fail_connect:
    close(queen_s);

fail_socket:
fail_receive_queen_addr:
fail_argp_parse:
    return rc;
}
