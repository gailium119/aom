/*
 * Copyright (c) 2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_FLOW_ESTIMATION_RANSAC_H_
#define AOM_FLOW_ESTIMATION_RANSAC_H_

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <memory.h>
#include <stdbool.h>

#include "aom_dsp/flow_estimation/flow_estimation.h"

#ifdef __cplusplus
extern "C" {
#endif

bool ransac(const Correspondence *matched_points, int npoints,
            TransformationType type, MotionModel *params_by_motion,
            int num_desired_motions);

// Fit a specified type of motion model to a set of correspondences.
// The input consists of `np` points, where pts1 stores the source position
// and pts2 stores the destination position for each correspondence.
// The resulting model is stored in `mat`
// Returns true on success, false on error
bool aom_fit_motion_model(TransformationType type, int np, const double *pts1,
                          const double *pts2, double *mat);

#ifdef __cplusplus
}
#endif

#endif  // AOM_FLOW_ESTIMATION_RANSAC_H_
