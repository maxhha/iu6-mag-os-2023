#include "templates.h"

#ifndef T
#define T int
#endif

typedef struct TEMPLATE3(vec, T, s) {
    int capacity;
    int size;
    T *data;
} * TEMPLATE3(vec, T, p);

TEMPLATE3(vec, T, p) TEMPLATE(create_vec, T) (void);

int TEMPLATE(append_vec, T) (TEMPLATE3(vec, T, p) v, T val);

void TEMPLATE(free_vec, T) (TEMPLATE3(vec, T, p) v);
