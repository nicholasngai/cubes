#ifndef ROTATIONS_H
#define ROTATIONS_H

#include <stdbool.h>

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

#define NUM_ROTATIONS (sizeof(rotations_list) / sizeof(*rotations_list))

#endif
