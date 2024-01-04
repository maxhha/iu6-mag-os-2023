#pragma once

#include <sys/poll.h>

#define MAX_EMMETS 200
#define MAX_DUTIES 10

#define DUTY_TYPE_EMPTY 0xD800
#define DUTY_TYPE_HEAR_PLEAS 0xD801  // read stdin
#define DUTY_TYPE_DELEGATE 0xD802

#define DUTY_STATE_WAITING 0x5701
#define DUTY_STATE_PROCESSING 0x5702

#define EMMET_STATE_EMPTY 0x3500
#define EMMET_STATE_CHILLING 0x3501
#define EMMET_STATE_HARD_WORKING 0x3502

typedef struct duty_s {
    int type;
    int state;
    int emmet_n;
} *duty_p;

typedef struct emmet_s {
    int processing_duty_i;
    int state;
} *emmet_p;

typedef struct queen_state_s {
    const int queen_s; // queen socket

    struct emmet_s emmets[MAX_EMMETS];
    int emmets_n;

    struct pollfd fds[MAX_EMMETS + 1]; // at 0 is queen socket

    struct duty_s duties[MAX_DUTIES];
    int duties_n;



} *queen_state_p;
