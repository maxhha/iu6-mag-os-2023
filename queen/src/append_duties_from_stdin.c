#include <stdlib.h>
#include <stdio.h>

#include "../inc/append_duties_from_stdin.h"

int append_duties_from_stdin(vec_duty_p_p duties, int max_size) {
    // TODO: read data from stdin
    for (int i = 0; i < 10; i++) {
        duty_p d = (duty_p) malloc(sizeof(struct duty_s));
        d->state = DUTY_STATE_WAITING;
        d->size = max_size;
        d->input = (int *) malloc(sizeof(int) * d->size);
        d->result = NULL;

        for (int j = 0; j < d->size; j++) {
            d->input[j] = 10000 * i + j;
        }

        if (append_vec_duty_p(duties, d)) {
            fprintf(stderr, "append_duties_from_stdin: append_vec_duty_p\n");
            return 1;
        }
    }

    return 0;
}
