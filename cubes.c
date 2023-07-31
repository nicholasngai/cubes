#include <search.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cube_t.h"
#include "defs.h"
#include "rotations.h"

struct cube_stat {
    size_t count;
    void *cube_tree;
    cube_t *cube_list;
};

static struct cube_stat all_cubes[MAX_DIM];

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

static int cube_comparator(const void *a_, const void *b_) {
    const cube_t *a = a_;
    const cube_t *b = b_;
    if (a->x_len > b->x_len) {
        return 1;
    } else if (a->x_len < b->x_len) {
        return -1;
    } else if (a->y_len > b->y_len) {
        return 1;
    } else if (a->y_len < b->y_len) {
        return -1;
    } else if (a->z_len > b->z_len) {
        return 1;
    } else if (a->z_len < b->z_len) {
        return -1;
    } else {
        return memcmp(a->coords, b->coords, sizeof(a->coords));
    }
}

static void find_next_cubes_for_cube(const cube_t *cube,
        struct cube_stat *next_stat) {
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

    /* Allocate heap space for normalized cube since they are ultimately
     * placed on the stack. */
    cube_t *normalized = malloc(sizeof(*normalized));
    if (!normalized) {
        perror("malloc normalized");
        exit(EXIT_FAILURE);
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
                normalize_cube(&candidate, normalized);

                /* Try to insert normalized cube into the tree for the next
                 * size. */
                cube_t **inserted =
                    tsearch(normalized, &next_stat->cube_tree, cube_comparator);
                if (!inserted) {
                    perror("tsearch normalized");
                    exit(EXIT_FAILURE);
                }
                if (*inserted == normalized) {
                    /* If inserted, allocate a new normalized cube buffer. */
                    normalized = malloc(sizeof(*normalized));
                    if (!normalized) {
                        perror("malloc normalized");
                        exit(EXIT_FAILURE);
                    }

                    /* Increment found count. */
                    next_stat->count++;
                }
            }
        }
    }

    /* Free remaining extra normalized cube. */
    free(normalized);
}

static struct cube_stat *flatten_tree_action_cube_stat;
static size_t flatten_tree_action_list_index;
static void flatten_tree_action(const void *cube_, VISIT which, int depth UNUSED) {
    if (which != postorder && which != leaf) {
        return;
    }

    cube_t *const *cube = cube_;

    flatten_tree_action_cube_stat->cube_list[flatten_tree_action_list_index] =
        **cube;
    flatten_tree_action_list_index++;
}

static void find_next_cubes_for_size(size_t size) {
    struct cube_stat *cur_stat = &all_cubes[size - 1];
    struct cube_stat *next_stat = &all_cubes[size];

    /* Find next cubes. */
    for (size_t i = 0; i < cur_stat->count; i++) {
        find_next_cubes_for_cube(&cur_stat->cube_list[i], next_stat);
    }

    /* Flatten search tree into an array. */
    next_stat->cube_list = malloc(next_stat->count * sizeof(cube_t));
    if (!next_stat->cube_list) {
        perror("malloc next_stat cube_list");
        exit(EXIT_FAILURE);
    }
    flatten_tree_action_cube_stat = next_stat;
    flatten_tree_action_list_index = 0;
    twalk(next_stat->cube_tree, flatten_tree_action);

    /* Destroy search tree. */
    while (next_stat->cube_tree) {
        cube_t *cube = *((cube_t **) next_stat->cube_tree);
        tdelete(cube, &next_stat->cube_tree, cube_comparator);
        free(cube);
    }
}

int main(void) {
    /* First polycube: 1x1x1 single cube. */
    cube_t *first_cube = malloc(sizeof(cube_t));
    if (!first_cube) {
        perror("malloc first cube");
        exit(EXIT_FAILURE);
    }
    *first_cube = (cube_t) {
        .coords = { { { true } } },
        .x_len = 1,
        .y_len = 1,
        .z_len = 1,
    };

    /* Add first cube to list. */
    all_cubes[0].cube_list = first_cube;
    all_cubes[0].count = 1;

    /* Find cubes. */
    find_next_cubes_for_size(1);

    /* Free resources. */
    free(all_cubes[0].cube_list);
    free(all_cubes[1].cube_list);

    return 0;
}
