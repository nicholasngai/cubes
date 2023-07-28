#ifndef CUBE_T_H
#define CUBE_T_H

#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>

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

#define INITIAL_LIST_CAP 8

static inline cube_t *cube_list_append(cube_list_t *list) {
    if (list->len == list->cap) {
        size_t new_cap = list->cap ? list->cap * 2 : INITIAL_LIST_CAP;
        cube_t *new_list = realloc(list->list, new_cap * sizeof(*list->list));
        if (!new_list) {
            perror("realloc new list");
            exit(EXIT_FAILURE);
        }
        list->list = new_list;
        list->cap = new_cap;
    }

    list->len++;
    return &list->list[list->len - 1];
}

#undef INITIAL_LIST_CAP

#endif
