#include <stdlib.h>

#include "../inc/templates.h"

#ifndef T
#define T int
// #include "../inc/vec.tmpl.h"
#endif // T

TEMPLATE3(vec, T, p) TEMPLATE(create_vec, T) (void) {
    int capacity = 8;
    TEMPLATE3(vec, T, p) v = (TEMPLATE3(vec, T, p)) malloc(sizeof (struct TEMPLATE3(vec, T, s)));
    if (v == NULL) {
        goto fail_malloc;
    }
    v->capacity = capacity;
    v->size = 0;

    v->data = (T *) malloc(sizeof (T) * capacity);
    if (v->data == NULL) {
        goto fail_data;
    }
    return v;

fail_data:
    free(v);
fail_malloc:
    return NULL;
}

int TEMPLATE(append_vec, T) (TEMPLATE3(vec, T, p) v, T val) {
    // printf("append_vec %p %d\n", v, val);
    if (v->size < v->capacity) {
        // printf("simple_add\n");
        v->data[v->size] = val;
        v->size++;
        // printf("v->size %d\n", v->size);
        return 0;
    }
    int new_cap = v->capacity * 2;
    T *data = (T *) malloc(sizeof (T) * new_cap);
    if (data == NULL) {
        return 1;
    }
    memcpy(data, v->data, v->capacity * sizeof(T));

    free(v->data);
    v->data = data;
    v->capacity = new_cap;
    v->data[v->size] = val;
    v->size++;

    return 0;
}

void TEMPLATE(free_vec, T) (TEMPLATE3(vec, T, p) v) {
    free(v->data);
    free(v);
}
