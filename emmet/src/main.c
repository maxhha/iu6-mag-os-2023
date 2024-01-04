#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <argp.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

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

void report_connect(const struct sockaddr *sa, socklen_t salen) {
	char remote_host[256];
	char remote_port[256];
	int herr;

	if ((herr = getnameinfo(sa, salen,
	    remote_host, sizeof(remote_host),
	    remote_port, sizeof(remote_port),
	    0)) != 0) {
		fprintf(stderr,
	    "getnameinfo error\n");
        return;
	}

	printf("Queen at %s:%s\n", remote_host, remote_port);
}

int receive_queen_addr(int wait_sec, int bcast_port, struct sockaddr_in *queen_addr_p) {
    int rc = 0;
    int fd_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (fd_s < 0) {
        fprintf(stderr, "receive_queen_addr: socket: fail to create socket\n");
        return 1;
    }

    struct timeval tv = {
        .tv_sec = wait_sec,
        .tv_usec = 0,
    };
    if (setsockopt(fd_s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
        perror("receive_queen_addr: setsockopt");
        goto fail_setsockopt;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(bcast_port);
    if (bind(fd_s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
        perror("receive_queen_addr: bind");
        rc = 1;
        goto fail_bind;
    }

    struct sockaddr_storage server_addr = {0};
    socklen_t server_addr_len = sizeof(server_addr);

    printf("Waiting for broadcast message on port %d...\n", bcast_port);

    in_port_t port;
    int read_n = recvfrom(fd_s, &port, sizeof(port), 0, (struct sockaddr *) &server_addr, &server_addr_len);
    if (read_n < 0) {
        perror("receive_queen_addr: recvfrom");
        rc = 1;
        goto fail_recvfrom;
    }

    char remote_host[256];
	char remote_port[256];
	int herr;

	if ((herr = getnameinfo((struct sockaddr *) &server_addr, server_addr_len,
	    remote_host, sizeof(remote_host),
	    remote_port, sizeof(remote_port),
	    0)) != 0) {
		fprintf(stderr, "receive_queen_addr: getnameinfo err %d\n", herr);
        printf("Receive port in message: %d\n", ntohs(port));
	} else {
        printf("Receive message from %s:%s\n", remote_host, remote_port);
        printf("Receive port in message: %d\n", ntohs(port));
        printf("Queen should be at %s:%d\n", remote_host, ntohs(port));
    }

    if (sizeof(struct sockaddr_in) != server_addr_len) {
        fprintf(stderr, "Queen addr not match to sizeof (struct sockaddr_in)\n");
        rc = 1;
        goto fail_serveraddrlen;
    }

    memset(queen_addr_p, 0, sizeof(struct sockaddr_in));
    queen_addr_p->sin_family = AF_INET;
    queen_addr_p->sin_addr =((struct sockaddr_in *) &server_addr)->sin_addr;
    queen_addr_p->sin_port = port;

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

fail_setsockopt:
fail_bind:
fail_recvfrom:
fail_serveraddrlen:
    close(fd_s);

    return rc;
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

//     int fd_s = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
//     if (fd_s < 0) {
//         fprintf(stderr, "main: socket: fail to create socket\n");
//         return 1;
//     }

//     struct timeval tv = {
//         .tv_sec = args.wait,
//         .tv_usec = 0,
//     };
//     if (setsockopt(fd_s, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv))) {
//         perror("main: setsockopt");
//         goto fail_setsockopt;
//     }

//     struct sockaddr_in addr = {0};
//     addr.sin_family = AF_INET;
//     addr.sin_port = htons(args.port);
//     if (bind(fd_s, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
//         perror("main: bind socket");
//         rc = 1;
//         goto fail_bind;
//     }

//     struct sockaddr_storage server_addr = {0};
//     socklen_t server_addr_len = sizeof(server_addr);

//     printf("Waiting for message...\n");

//     in_port_t port;
//     int read_n = recvfrom(fd_s, &port, sizeof(port), 0, (struct sockaddr *) &server_addr, &server_addr_len);
//     if (read_n < 0) {
//         perror("main: recvfrom");
//         rc = 1;
//         goto fail_recvfrom;
//     }

//     printf("Receive port in message: %d\n", ntohs(port));
//     report_connect((struct sockaddr *) &server_addr, server_addr_len);

//     /*

//         Emmet creates tcp connection to queen.
//         Queen gathers available emmets for work.
//         Queen splits and sends data for work to emmets.
//         Queen pings emmets. If someone failed
//           ж  ж  ж  ж  ж  .  ?
//         // print ж for every working emmet
//         // print ? for every disappeared worker
//         // print . when worker is free
//         // when data returned
//     */

// fail_setsockopt:
// fail_bind:
// fail_recvfrom:
//     close(fd_s);

fail_receive_queen_addr:
fail_argp_parse:
    return rc;
}
