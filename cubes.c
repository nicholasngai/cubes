#include <stdbool.h>
#include <stddef.h>
#include "cube_t.h"

static cube_list_t cubes_by_count[MAX_DIM];

static void find_next_cubes(const cube_t *cube) {
    /* Clone a new polycube shifted (1, 1, 1) in the positive direction to
     * create a gap all around the cube. */
    cube_t shifted = {
        .x_len = cube->x_len,
        .y_len = cube->y_len,
        .z_len = cube->z_len,
    };
    for (size_t i = 0; i < cube->x_len; i++) {
        for (size_t j = 0; j < cube->y_len; j++) {
            for (size_t k = 0; k < cube->z_len; k++) {
                shifted.coords[i + 1][j + 1][k + 1] = cube->coords[i][j][k];
            }
        }
    }

    (void) shifted;
}

int main(void) {
    /* First polycube: 1x1x1 single cube. */
    cube_t *first_cube = cube_list_append(&cubes_by_count[0]);
    *first_cube = (cube_t) {
        .coords = { { { true } } },
        .x_len = 1,
        .y_len = 1,
        .z_len = 1,
    };

    find_next_cubes(first_cube);

    return 0;
}
