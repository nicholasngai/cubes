#ifndef CUBE_T_H
#define CUBE_T_H

#include <stdbool.h>
#include <stddef.h>

#define MAX_DIM 20

typedef struct cube {
    bool coords[MAX_DIM + 1][MAX_DIM + 1][MAX_DIM + 1];
    size_t x_len;
    size_t y_len;
    size_t z_len;
} cube_t;

typedef struct cube_list {
    cube_t *list;
    size_t len;
    size_t cap;
} cube_list_t;

#endif
