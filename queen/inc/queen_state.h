#pragma once

#include <sys/poll.h>

#define MAX_EMMETS 200
#define MAX_DUTIES 10


#define DUTY_STATE_WAITING 0xD501
#define DUTY_STATE_PROCESSING 0xD502
#define DUTY_STATE_FINISHED 0xD503


#define EMMET_STATE_EMPTY 0x3500
#define EMMET_STATE_CHILLING 0x3501
#define EMMET_STATE_HARD_WORKING 0x3502
#define EMMET_STATE_CARRYING_RESULT 0x03503

typedef struct duty_s {
    int state;
    int size;
    int *input;
    int *result;
    // int *result;
} *duty_p;

void free_duty(duty_p d);

#define T duty_p
#define FREE_T() free_duty
#include "vec.tmpl.h"
#undef FREE_T
#undef T

typedef struct emmet_s {
    int state;

    // EMMET_STATE_HARD_WORKING
    // EMMET_STATE_CARRYING_RESULT
    int processing_duty_i;

    // EMMET_STATE_CARRYING_RESULT
    size_t received_result_n;
    char *result_buf;
} *emmet_p;

typedef struct queen_state_s {
    int queen_s; // queen socket

    struct emmet_s emmets[MAX_EMMETS];
    int emmets_n;

    struct pollfd fds[MAX_EMMETS + 1]; // at 0 is queen socket

    vec_duty_p_p duties;
    int finished_duties_n;

} *queen_state_p;


queen_state_p create_queen_state(int queen_socket);

int run_queen(queen_state_p state, int timeout);

void free_queen_state(queen_state_p state);
