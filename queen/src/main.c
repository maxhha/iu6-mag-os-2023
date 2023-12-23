#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <string.h>
#include <argp.h>
#include <unistd.h>

#include "../inc/main.h"

typedef struct arguments_s {
    char *address;
    int port;
    int wait;
} *arguments_p;

static char doc[] = "Queen of the colony";
static char args_doc[] = "[-a ADDR] [-p PORT]"; // [-w SECONDS]";

static struct argp_option options[] = {
    {"address", 'a', "ADDR", 0, "Broadcast address of the colony (default: 255.255.255.255)", 0},
    {"port", 'p', "PORT", 0, "Port of the colony (default: 12345)", 0},
    {"wait", 'w', "SECONDS", 0, "How long should wait for the emmets (default: 10)", 0},
    {0}
};

static int parse_opt(int key, char *arg, struct argp_state *state) {
    arguments_p args = (arguments_p) state->input;

    switch (key)
    {
    case 'a':
        if (inet_addr(arg) == -1) {
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

static struct argp argp = { options, parse_opt, args_doc, doc };

int main(int argc, char **argv) {
    int rc = 0;
    struct arguments_s args = {
        .address = "255.255.255.255",
        .port = 12345,
    };
    rc = argp_parse(&argp, argc, argv, 0, NULL, &args);
    if (rc != 0) {
        goto fail_argp_parse;
    }

    int bcast_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (bcast_s < 0) {
        fprintf(stderr, "main: socket: fail to create bcast_s\n");
        return 1;
    }
    int broadcast_enable = 1;
    if (setsockopt(bcast_s, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable))) {
        perror("main: setsockopt");
        rc = 1;
        goto fail_setsockopt;
    }

    struct sockaddr_in colony_addr = {0};
    colony_addr.sin_family = AF_INET;
    colony_addr.sin_addr.s_addr = inet_addr(args.address);
    colony_addr.sin_port = htons(args.port);

    char *message = "Hello my queen!";

    ssize_t sent_n = sendto(bcast_s, message, strlen(message) + 1, 0, (struct sockaddr *) &colony_addr, sizeof(colony_addr));
    if (sent_n < 0) {
        perror("main: sendto failed");
        rc = 1;
        goto fail_sendto;
    }

    printf("Sent %ld\n", sent_n);

fail_sendto:
fail_setsockopt:
    close(bcast_s);

fail_argp_parse:
    return rc;
}
