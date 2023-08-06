#ifndef CUBE_T_H
#define CUBE_T_H

#include <stddef.h>

#define MAX_DIM 20

typedef struct cube {
    unsigned char coords[MAX_DIM][3];
} cube_t;

typedef struct cube_list {
    cube_t *list;
    size_t len;
    size_t cap;
} cube_list_t;

#endif
