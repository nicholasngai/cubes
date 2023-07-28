#include <stdbool.h>
#include <stddef.h>
#include "cube_t.h"

static cube_list_t cubes_by_count[MAX_DIM];

static void find_next_cubes(const cube_t *cube) {
    /* Clone 3 new polycubes each shifted 1 in a positive direction to create a
     * gap all around the cube. */
    cube_t shifted_x = {
        .x_len = cube->x_len + 1,
        .y_len = cube->y_len,
        .z_len = cube->z_len,
    };
    cube_t shifted_y = {
        .x_len = cube->x_len,
        .y_len = cube->y_len + 1,
        .z_len = cube->z_len,
    };
    cube_t shifted_z = {
        .x_len = cube->x_len,
        .y_len = cube->y_len,
        .z_len = cube->z_len + 1,
    };
    for (size_t i = 0; i < cube->x_len; i++) {
        for (size_t j = 0; j < cube->y_len; j++) {
            for (size_t k = 0; k < cube->z_len; k++) {
                shifted_x.coords[i + 1][j][k] = cube->coords[i][j][k];
                shifted_y.coords[i][j + 1][k] = cube->coords[i][j][k];
                shifted_z.coords[i][j][k + 1] = cube->coords[i][j][k];
            }
        }
    }

    /* Try inserting a new cube at each potential position and insert it into
     * the next list if the cube may be placed there. A cube may only be placed
     * if it is adjacent to another cube. */
    for (size_t i = 0; i < cube->x_len + 2; i++) {
        for (size_t j = 0; j < cube->y_len + 2; j++) {
            for (size_t k = 0; k < cube->z_len + 2; k++) {
                /* Skip locations already populated. */
                if (i > 0 && j > 0 && k > 0
                        && cube->coords[i - 1][j - 1][k - 1]) {
                    continue;
                }

                /* Skip locations not adjacent to a cube. */
                if (!(i > 1 && j > 0 && k > 0 && cube->coords[i - 2][j - 1][k - 1])
                        && !(i > 0 && j > 1 && k > 0 && cube->coords[i - 1][j - 2][k - 1])
                        && !(i > 0 && j > 0 && k > 1 && cube->coords[i - 1][j - 1][k - 2])
                        && !(j > 0 && k > 0 && cube->coords[i][j - 1][k - 1])
                        && !(i > 0 && k > 0 && cube->coords[i - 1][j][k - 1])
                        && !(i > 0 && j > 0 && cube->coords[i - 1][j - 1][k])) {
                    continue;
                }

                /* Construct candidate polycube and mark the location. */
                cube_t candidate;
                if (i == 0) {
                    candidate = shifted_x;
                    candidate.coords[i][j - 1][k - 1] = true;
                } else if (j == 0) {
                    candidate = shifted_y;
                    candidate.coords[i - 1][j][k - 1] = true;
                } else if (k == 0) {
                    candidate = shifted_z;
                    candidate.coords[i - 1][j - 1][k] = true;
                } else {
                    candidate = *cube;
                    candidate.coords[i - 1][j - 1][k - 1] = true;
                    if (i == cube->x_len + 1) {
                        candidate.x_len++;
                    } else if (j == cube->y_len + 1) {
                        candidate.y_len++;
                    } else if (k == cube->z_len + 1) {
                        candidate.z_len++;
                    }
                }

                (void) candidate;
            }
        }
    }
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
