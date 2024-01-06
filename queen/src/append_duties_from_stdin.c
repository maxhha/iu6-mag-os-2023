#include <stdlib.h>
#include <stdio.h>

#include "../inc/append_duties_from_stdin.h"

int append_duties_from_stdin(vec_duty_p_p duties, int max_size) {
    // TODO: read data from stdin
    int size = 0;
    int *buf = (int *) malloc(sizeof(int) * max_size);
    if (buf == NULL) {
        fprintf(stderr, "append_duties_from_stdin: failed to malloc buf\n");
        return 1;
    }

    for(;;)
    {
        int rc = scanf("%d", buf + size);
        if (rc == EOF) {
            break;
        }

        if (rc != 1) {
            fprintf(stderr, "append_duties_from_stdin: failed to read number\n");

            free(buf);
            return 1;
        }

        size++;
        if (size < max_size) {
            continue;
        }

        duty_p d = create_duty_from_input(buf, size);
        if (d == NULL) {
            free(buf);
            return 1;
        }

        if (append_vec_duty_p(duties, d)) {
            fprintf(stderr, "append_duties_from_stdin: append_vec_duty_p\n");
            free_duty(d);
            return 1;
        }

        buf = (int *) malloc(sizeof(int) * max_size);
        size = 0;
        if (buf == NULL) {
            fprintf(stderr, "append_duties_from_stdin: failed to malloc next buf\n");
            return 1;
        }
    }

    if (size == 0) {
        free(buf);
    } else {
        duty_p d = create_duty_from_input(buf, size);
        if (d == NULL) {
            free(buf);
            return 1;
        }

        if (append_vec_duty_p(duties, d)) {
            fprintf(stderr, "append_duties_from_stdin: last append_vec_duty_p\n");
            free_duty(d);
            return 1;
        }
    }

    return 0;
}
