#ifndef CUBE_T_H
#define CUBE_T_H

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include "defs.h"

#define MAX_DIM 20

typedef struct cube {
    unsigned char coords[CEIL_DIV(MAX_DIM * MAX_DIM * MAX_DIM, CHAR_BIT)];
    size_t x_len;
    size_t y_len;
    size_t z_len;
} cube_t;

typedef struct cube_list {
    cube_t *list;
    size_t len;
    size_t cap;
} cube_list_t;

static inline bool cube_get(const cube_t *cube, size_t x, size_t y, size_t z) {
    size_t bit_idx = (x * MAX_DIM + y) * MAX_DIM + z;
    return (cube->coords[bit_idx / CHAR_BIT] >> (bit_idx % CHAR_BIT)) & 1;
}

static inline void cube_set(cube_t *cube, size_t x, size_t y, size_t z) {
    size_t bit_idx = (x * MAX_DIM + y) * MAX_DIM + z;
    cube->coords[bit_idx / CHAR_BIT] |= 1u << (bit_idx % CHAR_BIT);
}

static inline void cube_clear(cube_t *cube, size_t x, size_t y, size_t z) {
    size_t bit_idx = (x * MAX_DIM + y) * MAX_DIM + z;
    cube->coords[bit_idx / CHAR_BIT] &= ~(1u << (bit_idx % CHAR_BIT));
}

#endif
