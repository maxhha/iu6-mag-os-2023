#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "../inc/parallel_sort.h"

#define MIN_PART_SIZE 100

#define _STR(X) #X
#define STR(X) _STR(X)
#define SWAP(a, b, T) { T t = a; a = b; b = t; }

typedef struct sort_part_s {
    int *value;
    int size;
    int pos;
}* sort_part_p;

void merge_sort(int *a, int l, int r) {
    if (r - l == 1) {
        return;
    }
    if (r - l == 2) {
        if (a[l] > a[r - 1]) {
            SWAP(a[l], a[r - 1], int)
        }
        return;
    }

    int m = (l + r) / 2;
    merge_sort(a, l, m);
    merge_sort(a, m, r);

    int l_i = l, r_i = m;
    while (l_i < r_i && r_i < r) {
        if (a[l_i] <= a[r_i]) {
            l_i++;
        } else {
            int t = a[r_i];
            memmove((void *) &a[l_i + 1], (void *) &a[l_i], sizeof (int) * (r_i - l_i));
            a[l_i] = t;
            r_i++;
            l_i++;
        }
    }
}

void *sort_routine(void *param) {
    sort_part_p p = param;
    merge_sort(p->value, 0, p->size);
    return NULL;
}

int parallel_sort(int *arr, int size, int n_threads) {
    int rc = 0;
    int *buf = (int *) malloc(sizeof(int) * size);
    if (buf == NULL) {
        fprintf(stderr, "parallel_sort: fail malloc\n");
        return 1;
    }
    memcpy(buf, arr, size * sizeof(int));

    int part_size = size / n_threads;
    if (part_size < MIN_PART_SIZE) {
        int new_threads = size < MIN_PART_SIZE ? 1 : size / MIN_PART_SIZE;
        fprintf(
            stderr,
            "parallel_sort: part_size is too small (%d < " STR(MIN_PART_SIZE) ") "
            "for %d threads so use %d threads\n",
            part_size,
            n_threads,
            new_threads
        );
        n_threads = 1;
        part_size = size;
    }

    pthread_t tids[n_threads];
    memset(&tids, 0, sizeof(pthread_t) * n_threads);

    struct sort_part_s sort_chunks[n_threads];

    for (int i = 0; i < n_threads; i++) {
        pthread_attr_t attr;
        pthread_attr_init(&attr);

        sort_chunks[i].pos = 0;
        sort_chunks[i].value = buf + part_size*i;
        sort_chunks[i].size = i + 1 < n_threads ? part_size : (size - part_size * i);

        rc = pthread_create(&tids[i], &attr, &sort_routine, &sort_chunks[i]);
        if (rc != 0) {
            perror("parallel_sort: pthread_create");
            break;
        }
    }

    for (int i = 0; i < n_threads; i++) {
        if (tids[i] != 0) {
            pthread_join(tids[i], NULL);
        }
    }

    if (rc == 0) {
        sort_part_p heap = sort_chunks;
        int n = n_threads;

        for (int i = 0; i < n; i++) {
            for (int j = i, pj = (j - 1) / 2; j > 0 && *heap[pj].value > *heap[j].value; j = pj, pj = (j - 1) / 2) {
                SWAP(heap[pj], heap[j], struct sort_part_s)
            }
        }

        while(n > 0)
        {
            *arr = *heap[0].value;
            arr++;

            // printf("%d\n", *heap[0].value);
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
                SWAP(heap[j], heap[swap_i], struct sort_part_s)
                j = swap_i;
            }
        }
    }

    free(buf);

    return rc;
}
