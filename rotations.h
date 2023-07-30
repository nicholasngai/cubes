#ifndef ROTATIONS_H
#define ROTATIONS_H

#include <stdbool.h>
#include <stddef.h>

#define ROTATION_X_AXIS 0
#define ROTATION_Y_AXIS 1
#define ROTATION_Z_AXIS 2

struct rotation_spec {
    /* Whether we start at the most-negative corner or the most-positive
     * corner. */
    bool x_neg;
    bool y_neg;
    bool z_neg;

    /* The order of iteration on the axes. We only need 2 here since we can
     * infer the third. */
    int axis_order[2];
};

static struct rotation_spec rotations_list[] = {
    { false, false, false, { ROTATION_X_AXIS, ROTATION_Y_AXIS } },
    { false, false, false, { ROTATION_Y_AXIS, ROTATION_Z_AXIS } },
    { false, false, false, { ROTATION_Z_AXIS, ROTATION_X_AXIS } },
    { true, false, false, { ROTATION_Y_AXIS, ROTATION_X_AXIS } },
    { true, false, false, { ROTATION_X_AXIS, ROTATION_Z_AXIS } },
    { true, false, false, { ROTATION_Z_AXIS, ROTATION_Y_AXIS } },
    { false, true, false, { ROTATION_Y_AXIS, ROTATION_X_AXIS } },
    { false, true, false, { ROTATION_X_AXIS, ROTATION_Z_AXIS } },
    { false, true, false, { ROTATION_Z_AXIS, ROTATION_Y_AXIS } },
    { false, false, true, { ROTATION_Y_AXIS, ROTATION_X_AXIS } },
    { false, false, true, { ROTATION_X_AXIS, ROTATION_Z_AXIS } },
    { false, false, true, { ROTATION_Z_AXIS, ROTATION_Y_AXIS } },
    { true, true, false, { ROTATION_X_AXIS, ROTATION_Y_AXIS } },
    { true, true, false, { ROTATION_Y_AXIS, ROTATION_Z_AXIS } },
    { true, true, false, { ROTATION_Z_AXIS, ROTATION_X_AXIS } },
    { true, false, true, { ROTATION_X_AXIS, ROTATION_Y_AXIS } },
    { true, false, true, { ROTATION_Y_AXIS, ROTATION_Z_AXIS } },
    { true, false, true, { ROTATION_Z_AXIS, ROTATION_X_AXIS } },
    { false, true, true, { ROTATION_X_AXIS, ROTATION_Y_AXIS } },
    { false, true, true, { ROTATION_Y_AXIS, ROTATION_Z_AXIS } },
    { false, true, true, { ROTATION_Z_AXIS, ROTATION_X_AXIS } },
    { true, true, true, { ROTATION_Y_AXIS, ROTATION_X_AXIS } },
    { true, true, true, { ROTATION_X_AXIS, ROTATION_Z_AXIS } },
    { true, true, true, { ROTATION_Z_AXIS, ROTATION_Y_AXIS } },
};

static inline void rotation_get_projection(struct rotation_spec *rot,
        size_t index, size_t x_len, size_t y_len, size_t z_len, size_t *proj_x,
        size_t *proj_y, size_t *proj_z) {
    if (rot->axis_order[0] == ROTATION_X_AXIS) {
        *proj_x = index / (y_len * z_len);
        if (rot->axis_order[1] == ROTATION_Y_AXIS) {
            *proj_y = index % (y_len * z_len) / z_len;
            *proj_z = index % z_len;
        } else {
            *proj_z = index % (y_len * z_len) / y_len;
            *proj_y = index % y_len;
        }
    } else if (rot->axis_order[0] == ROTATION_Y_AXIS) {
        *proj_y = index / (x_len * z_len);
        if (rot->axis_order[1] == ROTATION_X_AXIS) {
            *proj_x = index % (x_len * z_len) / z_len;
            *proj_z = index % z_len;
        } else {
            *proj_z = index % (x_len * z_len) / x_len;
            *proj_x = index % x_len;
        }
    } else {
        *proj_z = index / (x_len * y_len);
        if (rot->axis_order[1] == ROTATION_X_AXIS) {
            *proj_x = index % (x_len * y_len) / y_len;
            *proj_y = index % y_len;
        } else {
            *proj_y = index % (x_len * y_len) / x_len;
            *proj_x = index % x_len;
        }
    }

    if (rot->x_neg) {
        *proj_x = x_len - *proj_x - 1;
    }
    if (rot->y_neg) {
        *proj_y = y_len - *proj_y - 1;
    }
    if (rot->z_neg) {
        *proj_z = z_len - *proj_z - 1;
    }
}

#define NUM_ROTATIONS (sizeof(rotations_list) / sizeof(*rotations_list))

#endif
