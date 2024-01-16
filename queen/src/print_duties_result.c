
#include <stdlib.h>
#include <stdio.h>

#include "../inc/print_duties_result.h"

typedef struct printing_state_s {
    int pos;
    int size;
    int *value;
} *printing_state_p;

#define SWAP(a, b, T) { T t = a; a = b; b = t; }

void print_duties_result(vec_duty_p_p duties) {
    fprintf(stderr, "Printing result for %d duties...\n", duties->size);
    if (duties->size > 10) {
        fprintf(stderr, "HINT: increase duty_max_size to make it faster (current: %d)\n", duties->data[0]->size);
    }

    // TODO: use heap
    int n = duties->size;
    printing_state_p heap = (printing_state_p) malloc(sizeof(struct printing_state_s) * n);
    if (heap == NULL) {
        perror("print_duties_result: malloc");
        return;
    }

    size_t total_size = 0;
    size_t printed_n = 0;

    for (int i = 0; i < n; i++) {
        duty_p d = duties->data[i];

        heap[i].pos = 0;
        heap[i].size = d->size;
        heap[i].value = d->result;

        int j = i;
        for (int pj = (j - 1) / 2; j > 0 && *heap[pj].value > *heap[j].value; j = pj, pj = (j - 1) / 2) {
            SWAP(heap[pj], heap[j], struct printing_state_s)
        }

        total_size += d->size;
    }

    while(n > 0)
    {
        printf("%d\n", *heap[0].value);
        heap[0].pos++;
        heap[0].value++;
        if (heap[0].pos >= heap[0].size) {
            n--;
            heap[0] = heap[n];
        }

        int j = 0;
        while (j < n) {
            int a = j*2 + 1;
            int b = j*2 + 2;

            if ((a >= n || *heap[j].value < *heap[a].value) && (b >= n || *heap[j].value < *heap[b].value)) {
                break;
            }

            int swap_i = b >= n ? a : *heap[a].value < *heap[b].value ? a : b;
            SWAP(heap[j], heap[swap_i], struct printing_state_s)
            j = swap_i;
        }

        printed_n += 1;
        if (printed_n % (total_size / 10) == 0) {
            fprintf(stderr, "Progress: %.0f%%\n", (float) printed_n / total_size * 100);
        }
    }

    fprintf(stderr, "Done\n");
    free(heap);
}
