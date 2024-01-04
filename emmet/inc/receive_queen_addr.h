#pragma once

#include <arpa/inet.h>

int receive_queen_addr(int wait_sec, int bcast_port, struct sockaddr_in *queen_addr_p);
