#include <assert.h>
#include <errno.h>
#include <limits.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cube_t.h"
#include "defs.h"
#include "hash.h"
#include "rotations.h"

#define HASH_SIZE 4096

struct cube_stat {
    atomic_size_t count;
    struct hash cube_hash;
    cube_t *cube_list;
};

static struct cube_stat all_cubes[MAX_DIM];

struct cube_coords {
    coord_t coords[CEIL_DIV(MAX_DIM * MAX_DIM * MAX_DIM,  CHAR_BIT)];
    coord_t x_len;
    coord_t y_len;
    coord_t z_len;
};

static inline bool coord_get(const struct cube_coords *coords, coord_t x,
        coord_t y, coord_t z) {
    size_t bit_idx = (x * MAX_DIM + y) * MAX_DIM + z;
    return (coords->coords[bit_idx / CHAR_BIT] >> (bit_idx % CHAR_BIT)) & 1;
}

static inline void coord_set(struct cube_coords *coords, coord_t x, coord_t y,
        coord_t z) {
    size_t bit_idx = (x * MAX_DIM + y) * MAX_DIM + z;
    coords->coords[bit_idx / CHAR_BIT] |= 1u << (bit_idx % CHAR_BIT);
}

static void normalize_cube(const struct cube_coords *coords,
        struct cube_coords *normalized) {
    /* Iterate through all the rotations and find the lexicographically
     * earliest polycube according to coordinates in order to find "normalized"
     * form. We will do this by iterating down the coordinates of the polycube
     * in a manner corresponding to each of the 24 possible rotations defined
     * in rotations.h. */
    bool rotation_active[NUM_ROTATIONS];
    bool rotation_found[NUM_ROTATIONS];
    memset(rotation_active, true, sizeof(rotation_active));

    coord_t lengths_by_axis[] = { coords->x_len, coords->y_len, coords->z_len };

    struct rotation_spec *norm_rot = NULL;
    size_t found_count = 0;
    for (size_t index = 0;
            found_count != 1
                && index < coords->x_len * coords->y_len * coords->z_len;
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
            coord_t proj_x, proj_y, proj_z;
            rotation_get_projection(rot, index, coords->x_len, coords->y_len,
                    coords->z_len, &proj_x, &proj_y, &proj_z);

            if (coord_get(coords, proj_x, proj_y, proj_z)) {
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
    *normalized = (struct cube_coords) {
        .x_len = lengths_by_axis[norm_rot->axis_order[0]],
        .y_len = lengths_by_axis[norm_rot->axis_order[1]],
        .z_len = lengths_by_axis[3 - norm_rot->axis_order[0] - norm_rot->axis_order[1]],
    };

    for (coord_t x = 0; x < normalized->x_len; x++) {
        for (coord_t y = 0; y < normalized->y_len; y++) {
            for (coord_t z = 0; z < normalized->z_len; z++) {
                size_t index =
                    (x * normalized->y_len + y) * normalized->z_len + z;
                coord_t proj_x, proj_y, proj_z;
                rotation_get_projection(norm_rot, index, coords->x_len,
                        coords->y_len, coords->z_len, &proj_x, &proj_y, &proj_z);
                if (coord_get(coords, proj_x, proj_y, proj_z)) {
                    coord_set(normalized, x, y, z);
                }
            }
        }
    }
}

static void find_next_cubes_for_cube(const cube_t *cube, size_t size,
        struct cube_stat *next_stat) {
    /* Generate regular and shifted coordinates structures from polycube. There
     * will be 3 shifted polycubes each shifted 1 in a positive direction to
     * create a gap all around the polycube. */
    struct cube_coords orig = { .coords = { 0 } };
    struct cube_coords shifted_x = { .coords = { 0 } };
    struct cube_coords shifted_y = { .coords = { 0 } };
    struct cube_coords shifted_z = { .coords = { 0 } };
    coord_t max_x = 0;
    coord_t max_y = 0;
    coord_t max_z = 0;
    for (size_t i = 0; i < size; i++) {
        coord_t x = cube->coords[i][0];
        coord_t y = cube->coords[i][1];
        coord_t z = cube->coords[i][2];
        coord_set(&orig, x, y, z);
        coord_set(&shifted_x, x + 1, y, z);
        coord_set(&shifted_y, x, y + 1, z);
        coord_set(&shifted_z, x, y, z + 1);
        if (x > max_x) {
            max_x = x;
        }
        if (y > max_y) {
            max_y = y;
        }
        if (z > max_z) {
            max_z = z;
        }
    }
    orig.x_len = max_x + 1;
    orig.y_len = max_y + 1;
    orig.z_len = max_z + 1;
    shifted_x.x_len = max_x + 2;
    shifted_x.y_len = max_y + 1;
    shifted_x.z_len = max_z + 1;
    shifted_y.x_len = max_x + 1;
    shifted_y.y_len = max_y + 2;
    shifted_y.z_len = max_z + 1;
    shifted_z.x_len = max_x + 1;
    shifted_z.y_len = max_y + 1;
    shifted_z.z_len = max_z + 2;

    /* Allocate heap space for normalized cube since they are ultimately
     * placed into the map. */
    cube_t *normalized = malloc(sizeof(*normalized));
    if (!normalized) {
        perror("malloc normalized");
        exit(EXIT_FAILURE);
    }

    /* Try inserting a new cube at each potential position and insert it into
     * the next list if the cube may be placed there. A cube may only be placed
     * if it is adjacent to another cube. */
    for (coord_t i = 0; i < orig.x_len + 2; i++) {
        for (coord_t j = 0; j < orig.y_len + 2; j++) {
            for (coord_t k = 0; k < orig.z_len + 2; k++) {
                /* Skip locations already populated. */
                if (i > 0 && j > 0 && k > 0
                        && coord_get(&orig, i - 1, j - 1, k - 1)) {
                    continue;
                }

                /* Skip locations not adjacent to a cube. */
                if (!(i > 1 && j > 0 && k > 0 && coord_get(&orig, i - 2, j - 1, k - 1))
                        && !(i > 0 && j > 1 && k > 0 && coord_get(&orig, i - 1, j - 2, k - 1))
                        && !(i > 0 && j > 0 && k > 1 && coord_get(&orig, i - 1, j - 1, k - 2))
                        && !(j > 0 && k > 0 && coord_get(&orig, i, j - 1, k - 1))
                        && !(i > 0 && k > 0 && coord_get(&orig, i - 1, j, k - 1))
                        && !(i > 0 && j > 0 && coord_get(&orig, i - 1, j - 1, k))) {
                    continue;
                }

                /* Construct candidate polycube and mark the location. */
                struct cube_coords candidate;
                if (i == 0) {
                    candidate = shifted_x;
                    coord_set(&candidate, i, j - 1, k - 1);
                } else if (j == 0) {
                    candidate = shifted_y;
                    coord_set(&candidate, i - 1, j, k - 1);
                } else if (k == 0) {
                    candidate = shifted_z;
                    coord_set(&candidate, i - 1, j - 1, k);
                } else {
                    candidate = orig;
                    coord_set(&candidate, i - 1, j - 1, k - 1);
                    if (i == orig.x_len + 1) {
                        candidate.x_len++;
                    } else if (j == orig.y_len + 1) {
                        candidate.y_len++;
                    } else if (k == orig.z_len + 1) {
                        candidate.z_len++;
                    }
                }

                /* Get normalized cube coords. */
                struct cube_coords normalized_coords;
                normalize_cube(&candidate, &normalized_coords);

                /* Get normalized cube from coords. */
                size_t coord_idx = 0;
                for (size_t x = 0; x < normalized_coords.x_len; x++) {
                    for (size_t y = 0; y < normalized_coords.y_len; y++) {
                        for (size_t z = 0; z < normalized_coords.z_len; z++) {
                            if (coord_get(&normalized_coords, x, y, z)) {
                                normalized->coords[coord_idx][0] = x;
                                normalized->coords[coord_idx][1] = y;
                                normalized->coords[coord_idx][2] = z;
                                coord_idx++;
                                if (coord_idx >= size + 1) {
                                    break;
                                }
                            }
                        }
                    }
                }

                /* Try to insert normalized cube into the tree for the next
                 * size. */
                cube_t *inserted =
                    hash_search(&next_stat->cube_hash, normalized->coords,
                            (size + 1) * sizeof(*normalized->coords),
                            normalized);
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
#pragma omp parallel for
    for (size_t i = 0; i < cur_stat->count; i++) {
        find_next_cubes_for_cube(&cur_stat->cube_list[i], size, next_stat);
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

static void usage(char **argv) {
    printf("Usage: %s <max size>\n", argv[0]);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv);
        exit(EXIT_FAILURE);
    }

    /* Parse args. */
    errno = 0;
    size_t max_size = strtoull(argv[1], NULL, 10);
    if (errno != 0) {
        perror("strtoull max_size");
        exit(EXIT_FAILURE);
    }
    if (max_size == 0) {
        printf("Max size must be greater than 0!\n");
        exit(EXIT_FAILURE);
    }

    /* First polycube: 1x1x1 single cube. */
    cube_t *first_cube = malloc(sizeof(cube_t));
    if (!first_cube) {
        perror("malloc first cube");
        exit(EXIT_FAILURE);
    }
    *first_cube = (cube_t) {
        .coords = { { 0, 0, 0 } },
    };

    /* Add first cube to list. */
    all_cubes[0].cube_list = first_cube;
    all_cubes[0].count = 1;
    printf("%2d: %zu\n", 1, all_cubes[0].count);

    /* Find cubes. */
    for (size_t size = 1; size < max_size; size++) {
        find_next_cubes_for_size(size);

        printf("%2zu: %zu\n", size + 1, all_cubes[size].count);
    }

    /* Free resources. */
    for (size_t size = 1; size <= max_size; size++) {
        free(all_cubes[size - 1].cube_list);
    }

    return 0;
}
