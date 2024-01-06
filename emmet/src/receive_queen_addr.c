#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <netdb.h>
#include <string.h>

#include "../inc/receive_queen_addr.h"

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

    fprintf(stderr, "Waiting for broadcast message on port %d...\n", bcast_port);

    in_port_t port;
    int read_n = recvfrom(fd_s, &port, sizeof(port), 0, (struct sockaddr *) &server_addr, &server_addr_len);
    if (read_n < 0) {
        perror("receive_queen_addr: recvfrom");
        rc = 1;
        goto fail_recvfrom;
    }

    char remote_host[256] = "unknown";
	char remote_port[256] = "unknown";
	int herr;

	if ((herr = getnameinfo((struct sockaddr *) &server_addr, server_addr_len,
	    remote_host, sizeof(remote_host),
	    remote_port, sizeof(remote_port),
	    0)) != 0) {
		fprintf(stderr, "receive_queen_addr: getnameinfo err %d\n", herr);
	}
    fprintf(stderr, "Receive message from %s:%s\n", remote_host, remote_port);
    fprintf(stderr, "Receive port in message: %d\n", ntohs(port));
    fprintf(stderr, "Queen should be at %s:%d\n", remote_host, ntohs(port));

    if (sizeof(struct sockaddr_in) != server_addr_len) {
        fprintf(stderr, "Queen addr not match to sizeof (struct sockaddr_in)\n");
        rc = 1;
        goto fail_serveraddrlen;
    }

    memset(queen_addr_p, 0, sizeof(struct sockaddr_in));
    queen_addr_p->sin_family = AF_INET;
    queen_addr_p->sin_addr =((struct sockaddr_in *) &server_addr)->sin_addr;
    queen_addr_p->sin_port = port;

fail_setsockopt:
fail_bind:
fail_recvfrom:
fail_serveraddrlen:
    close(fd_s);

    return rc;
}
