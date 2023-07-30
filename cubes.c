#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include "cube_t.h"
#include "rotations.h"

static cube_list_t cubes_by_count[MAX_DIM];

static void normalize_cube(const cube_t *restrict cube,
        cube_t *restrict normalized) {
    /* Iterate through all the rotations and find the lexicographically
     * earliest polycube according to coordinates in order to find "normalized"
     * form. We will do this by iterating down the coordinates of the polycube
     * in a manner corresponding to each of the 24 possible rotations defined
     * in rotations.h. */
    bool rotation_active[NUM_ROTATIONS];
    bool rotation_found[NUM_ROTATIONS];
    memset(rotation_active, true, sizeof(rotation_active));

    size_t lengths_by_axis[] = { cube->x_len, cube->y_len, cube->z_len };

    struct rotation_spec *norm_rot;
    size_t found_count = 0;
    for (size_t index = 0;
            found_count != 1
                && index < cube->x_len * cube->y_len * cube->z_len;
            index++) {
        memset(rotation_found, false, sizeof(rotation_found));
        found_count = 0;

        for (size_t i = 0; i < NUM_ROTATIONS; i++) {
            /* Skip rotations marked to skip in previous iterations. */
            if (!rotation_active[i]) {
                continue;
            }

            struct rotation_spec *rot = &rotations_list[i];

            /* Skip rotations where the lengths of axes in order are not in
             * descending order. */
            if (lengths_by_axis[rot->axis_order[0]]
                        < lengths_by_axis[rot->axis_order[1]]
                    || lengths_by_axis[rot->axis_order[1]]
                        < lengths_by_axis[3 - rot->axis_order[0] - rot->axis_order[1]]) {
                rotation_active[i] = false;
                continue;
            }

            /* Get projected rotation coordinates onto the current view of the
             * polycube. */
            size_t proj_x, proj_y, proj_z;
            rotation_get_projection(rot, index, cube->x_len, cube->y_len,
                    cube->z_len, &proj_x, &proj_y, &proj_z);

            if (cube->coords[proj_x][proj_y][proj_z]) {
                rotation_found[i] = true;
                found_count++;
                if (found_count == 1) {
                    norm_rot = rot;
                }
            }
        }

        if (found_count > 1) {
            for (size_t i = 0; i < NUM_ROTATIONS; i++) {
                if (!rotation_active[i]) {
                    continue;
                }

                if (!rotation_found[i]) {
                    rotation_active[i] = false;
                }
            }
        }
    }

    /* Build normalized cube. */
    *normalized = (cube_t) {
        .coords = { { { 0 } } },
        .x_len = lengths_by_axis[norm_rot->axis_order[0]],
        .y_len = lengths_by_axis[norm_rot->axis_order[1]],
        .z_len = lengths_by_axis[3 - norm_rot->axis_order[0] - norm_rot->axis_order[1]],
    };

    for (size_t x = 0; x < normalized->x_len; x++) {
        for (size_t y = 0; y < normalized->y_len; y++) {
            for (size_t z = 0; z < normalized->z_len; z++) {
                size_t index =
                    (x * normalized->y_len + y) * normalized->z_len + z;
                size_t proj_x, proj_y, proj_z;
                rotation_get_projection(norm_rot, index, cube->x_len,
                        cube->y_len, cube->z_len, &proj_x, &proj_y, &proj_z);
                normalized->coords[x][y][z] =
                    cube->coords[proj_x][proj_y][proj_z];
            }
        }
    }
}

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

                /* Get normalized cube. */
                cube_t normalized;
                normalize_cube(&candidate, &normalized);
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
