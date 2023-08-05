#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "cube_t.h"
#include "defs.h"
#include "hash.h"
#include "rotations.h"

#define HASH_SIZE 256

struct cube_stat {
    size_t count;
    struct hash cube_hash;
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

    struct rotation_spec *norm_rot = NULL;
    size_t found_count = 0;
    for (size_t index = 0;
            found_count != 1
                && index < cube->x_len * cube->y_len * cube->z_len;
            index++) {
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

            if (cube_get(cube, proj_x, proj_y, proj_z)) {
                rotation_found[i] = true;
                found_count++;
                if (found_count == 1) {
                    norm_rot = rot;
                }
            } else {
                rotation_found[i] = false;
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

    assert(norm_rot != NULL);

    /* Build normalized cube. */
    *normalized = (cube_t) {
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
                if (cube_get(cube, proj_x, proj_y, proj_z)) {
                    cube_set(normalized, x, y, z);
                }
            }
        }
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
                if (cube_get(cube, i, j, k)) {
                    cube_set(&shifted_x, i + 1, j, k);
                    cube_set(&shifted_y, i, j + 1, k);
                    cube_set(&shifted_z, i, j, k + 1);
                }
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
                        && cube_get(cube, i - 1, j - 1, k - 1)) {
                    continue;
                }

                /* Skip locations not adjacent to a cube. */
                if (!(i > 1 && j > 0 && k > 0 && cube_get(cube, i - 2, j - 1, k - 1))
                        && !(i > 0 && j > 1 && k > 0 && cube_get(cube, i - 1, j - 2, k - 1))
                        && !(i > 0 && j > 0 && k > 1 && cube_get(cube, i - 1, j - 1, k - 2))
                        && !(j > 0 && k > 0 && cube_get(cube, i, j - 1, k - 1))
                        && !(i > 0 && k > 0 && cube_get(cube, i - 1, j, k - 1))
                        && !(i > 0 && j > 0 && cube_get(cube, i - 1, j - 1, k))) {
                    continue;
                }

                /* Construct candidate polycube and mark the location. */
                cube_t candidate;
                if (i == 0) {
                    candidate = shifted_x;
                    cube_set(&candidate, i, j - 1, k - 1);
                } else if (j == 0) {
                    candidate = shifted_y;
                    cube_set(&candidate, i - 1, j, k - 1);
                } else if (k == 0) {
                    candidate = shifted_z;
                    cube_set(&candidate, i - 1, j - 1, k);
                } else {
                    candidate = *cube;
                    cube_set(&candidate, i - 1, j - 1, k - 1);
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
                cube_t *inserted =
                    hash_search(&next_stat->cube_hash, normalized->coords,
                            sizeof(normalized->coords), normalized);
                if (!inserted) {
                    perror("hash_search normalized");
                    exit(EXIT_FAILURE);
                }
                if (inserted == normalized) {
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

struct flatten_hash_callback_aux {
    cube_t *list;
};
static void flatten_hash_callback(const void *coords UNUSED,
        size_t coords_len UNUSED, void *cube_, void *aux_) {
    cube_t *cube = cube_;
    struct flatten_hash_callback_aux *aux = aux_;
    *aux->list = *cube;
    aux->list++;
    free(cube);
}

static void find_next_cubes_for_size(size_t size) {
    struct cube_stat *cur_stat = &all_cubes[size - 1];
    struct cube_stat *next_stat = &all_cubes[size];

    /* Allocate next cube hash. */
    if (hash_init(&next_stat->cube_hash, HASH_SIZE)) {
        perror("hash_init");
        exit(EXIT_FAILURE);
    }

    /* Find next cubes. */
    for (size_t i = 0; i < cur_stat->count; i++) {
        find_next_cubes_for_cube(&cur_stat->cube_list[i], next_stat);
    }

    /* Flatten hash into an array and destroy the hash. */
    next_stat->cube_list = malloc(next_stat->count * sizeof(cube_t));
    if (!next_stat->cube_list) {
        perror("malloc next_stat cube_list");
        exit(EXIT_FAILURE);
    }
    struct flatten_hash_callback_aux flatten_hash_callback_aux = {
        .list = next_stat->cube_list,
    };
    hash_free(&next_stat->cube_hash, flatten_hash_callback,
            &flatten_hash_callback_aux);
}

static void dump_cubes(size_t size, const struct cube_stat *stat) {
    printf("size = %zu\n\n", size);
    for (size_t i = 0; i < stat->count; i++) {
        cube_t *cube = &stat->cube_list[i];
        for (size_t y = 0; y < cube->y_len; y++) {
            for (size_t z = 0; z < cube->z_len; z++) {
                if (z > 0) {
                    printf(" ");
                }
                for (size_t x = 0; x < cube->x_len; x++) {
                    printf("%d", cube_get(cube, x, y, z));
                }
            }
            printf("\n");
        }
        printf("\n");
    }
}

static void usage(char **argv) {
    printf("Usage: %s [-d] <max size>\n", argv[0]);
    printf("Usage: %s -h\n", argv[0]);
}

int main(int argc, char **argv) {
    /* Parse optargs. */
    bool should_dump_cubes = false;
    int opt;
    while ((opt = getopt(argc, argv, "dh")) != -1) {
        switch (opt) {
        case 'd':
            should_dump_cubes = true;
            break;
        case 'h':
        default:
            usage(argv);
            exit(0);
            break;
        }
    }

    if (argc - optind + 1 < 2) {
        usage(argv);
        exit(EXIT_FAILURE);
    }

    /* Parse args. */
    errno = 0;
    size_t max_size = strtoull(argv[optind], NULL, 10);
    if (errno != 0) {
        perror("strtoull max_size");
        exit(EXIT_FAILURE);
    }
    if (max_size == 0) {
        printf("Max size must be greater than 0!\n");
        exit(EXIT_FAILURE);
    }
    optind++;

    /* First polycube: 1x1x1 single cube. */
    cube_t *first_cube = malloc(sizeof(cube_t));
    if (!first_cube) {
        perror("malloc first cube");
        exit(EXIT_FAILURE);
    }
    *first_cube = (cube_t) {
        .x_len = 1,
        .y_len = 1,
        .z_len = 1,
    };
    cube_set(first_cube, 0, 0, 0);

    /* Add first cube to list. */
    all_cubes[0].cube_list = first_cube;
    all_cubes[0].count = 1;
    printf("%2d: %zu\n", 1, all_cubes[0].count);
    if (should_dump_cubes) {
        dump_cubes(1, &all_cubes[0]);
    }

    /* Find cubes. */
    for (size_t size = 1; size < max_size; size++) {
        find_next_cubes_for_size(size);

        printf("%2zu: %zu\n", size + 1, all_cubes[size].count);

        if (should_dump_cubes) {
            dump_cubes(size + 1, &all_cubes[size]);
        }
    }

    /* Free resources. */
    for (size_t size = 1; size <= max_size; size++) {
        free(all_cubes[size - 1].cube_list);
    }

    return 0;
}
