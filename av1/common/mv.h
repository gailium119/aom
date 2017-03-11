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

#ifndef AV1_COMMON_MV_H_
#define AV1_COMMON_MV_H_

#include "av1/common/common.h"
#include "av1/common/common_data.h"
#include "aom_dsp/aom_filter.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct mv {
  int16_t row;
  int16_t col;
} MV;

typedef union int_mv {
  uint32_t as_int;
  MV as_mv;
} int_mv; /* facilitates faster equality tests and copies */

typedef struct mv32 {
  int32_t row;
  int32_t col;
} MV32;

#if (CONFIG_WARPED_MOTION || CONFIG_MOTION_VAR) && CONFIG_GLOBAL_MOTION
#define SEPARATE_GLOBAL_MOTION 0
#endif  // (CONFIG_WARPED_MOTION || CONFIG_MOTION_VAR) && CONFIG_GLOBAL_MOTION
#if CONFIG_GLOBAL_MOTION || CONFIG_WARPED_MOTION
// Bits of precision used for the model
#define WARPEDMODEL_PREC_BITS 16
#define WARPEDMODEL_ROW3HOMO_PREC_BITS 16

// Bits of subpel precision for warped interpolation
#define WARPEDPIXEL_PREC_BITS 6
#define WARPEDPIXEL_PREC_SHIFTS (1 << WARPEDPIXEL_PREC_BITS)

// Taps for ntap filter
#define WARPEDPIXEL_FILTER_TAPS 6

// Precision of filter taps
#define WARPEDPIXEL_FILTER_BITS 7

#define WARPEDDIFF_PREC_BITS (WARPEDMODEL_PREC_BITS - WARPEDPIXEL_PREC_BITS)

/* clang-format off */
typedef enum {
  IDENTITY = 0,      // identity transformation, 0-parameter
  TRANSLATION = 1,   // translational motion 2-parameter
  ROTZOOM = 2,       // simplified affine with rotation + zoom only, 4-parameter
  AFFINE = 3,        // affine, 6-parameter
  HORTRAPEZOID = 4,  // constrained homography, hor trapezoid, 6-parameter
  VERTRAPEZOID = 5,  // constrained homography, ver trapezoid, 6-parameter
  HOMOGRAPHY = 6,    // homography, 8-parameter
  TRANS_TYPES = 7,
} TransformationType;
/* clang-format on */

// Number of types used for global motion (must be >= 3 and <= TRANS_TYPES)
// The following can be useful:
// GLOBAL_TRANS_TYPES 3 - up to rotation-zoom
// GLOBAL_TRANS_TYPES 4 - up to affine
// GLOBAL_TRANS_TYPES 6 - up to hor/ver trapezoids
// GLOBAL_TRANS_TYPES 7 - up to full homography
#define GLOBAL_TRANS_TYPES 3

// number of parameters used by each transformation in TransformationTypes
static const int trans_model_params[TRANS_TYPES] = { 0, 2, 4, 6, 6, 6, 8 };

// The order of values in the wmmat matrix below is best described
// by the homography:
//      [x'     (m2 m3 m0   [x
//  z .  y'  =   m4 m5 m1 *  y
//       1]      m6 m7 1)    1]
typedef struct {
  TransformationType wmtype;
  int32_t wmmat[8];
} WarpedMotionParams;
#endif  // CONFIG_GLOBAL_MOTION || CONFIG_WARPED_MOTION

#if CONFIG_GLOBAL_MOTION
// ALPHA here refers to parameters a and b in rotzoom model:
// | a   b|
// |-b   a|
//
// and a, b, c, d in affine model:
// | a   b|
// | c   d|
//
// Anything ending in PREC_BITS is the number of bits of precision
// to maintain when converting from double to integer.
//
// The ABS parameters are used to create an upper and lower bound
// for each parameter. In other words, after a parameter is integerized
// it is clamped between -(1 << ABS_XXX_BITS) and (1 << ABS_XXX_BITS).
//
// XXX_PREC_DIFF and XXX_DECODE_FACTOR
// are computed once here to prevent repetitive
// computation on the decoder side. These are
// to allow the global motion parameters to be encoded in a lower
// precision than the warped model precision. This means that they
// need to be changed to warped precision when they are decoded.
//
// XX_MIN, XX_MAX are also computed to avoid repeated computation

#define GM_TRANS_PREC_BITS 6
#define GM_ABS_TRANS_BITS 12
#define GM_ABS_TRANS_ONLY_BITS (GM_ABS_TRANS_BITS - GM_TRANS_PREC_BITS + 3)
#define GM_TRANS_PREC_DIFF (WARPEDMODEL_PREC_BITS - GM_TRANS_PREC_BITS)
#define GM_TRANS_ONLY_PREC_DIFF (WARPEDMODEL_PREC_BITS - 3)
#define GM_TRANS_DECODE_FACTOR (1 << GM_TRANS_PREC_DIFF)
#define GM_TRANS_ONLY_DECODE_FACTOR (1 << GM_TRANS_PREC_DIFF)

#define GM_ALPHA_PREC_BITS 15
#define GM_ABS_ALPHA_BITS 12
#define GM_ALPHA_PREC_DIFF (WARPEDMODEL_PREC_BITS - GM_ALPHA_PREC_BITS)
#define GM_ALPHA_DECODE_FACTOR (1 << GM_ALPHA_PREC_DIFF)

#define GM_ROW3HOMO_PREC_BITS 16
#define GM_ABS_ROW3HOMO_BITS 11
#define GM_ROW3HOMO_PREC_DIFF \
  (WARPEDMODEL_ROW3HOMO_PREC_BITS - GM_ROW3HOMO_PREC_BITS)
#define GM_ROW3HOMO_DECODE_FACTOR (1 << GM_ROW3HOMO_PREC_DIFF)

#define GM_TRANS_MAX (1 << GM_ABS_TRANS_BITS)
#define GM_ALPHA_MAX (1 << GM_ABS_ALPHA_BITS)
#define GM_ROW3HOMO_MAX (1 << GM_ABS_ROW3HOMO_BITS)

#define GM_TRANS_MIN -GM_TRANS_MAX
#define GM_ALPHA_MIN -GM_ALPHA_MAX
#define GM_ROW3HOMO_MIN -GM_ROW3HOMO_MAX

// Maximum number of bits used for different models
#define GM_IDENTITY_BITS 0
#define GM_TRANSLATION_BITS ((GM_ABS_TRANS_ONLY_BITS + 1) * 2)
#define GM_ROTZOOM_BITS (GM_TRANSLATION_BITS + (GM_ABS_ALPHA_BITS + 1) * 2)
#define GM_AFFINE_BITS (GM_ROTZOOM_BITS + (GM_ABS_ALPHA_BITS + 1) * 2)
#define GM_HOMOGRAPHY_BITS (GM_AFFINE_BITS + (GM_ABS_ROW3HOMO_BITS + 1) * 2)
#define GM_HORTRAPEZOID_BITS \
  (GM_AFFINE_BITS - GM_ABS_ALPHA_BITS + GM_ABS_ROW3HOMO_BITS)
#define GM_VERTRAPEZOID_BITS \
  (GM_AFFINE_BITS - GM_ABS_ALPHA_BITS + GM_ABS_ROW3HOMO_BITS)

// Use global motion parameters for sub8x8 blocks
#define GLOBAL_SUB8X8_USED 0

static INLINE int block_center_x(int mi_col, BLOCK_SIZE bs) {
  const int bw = block_size_wide[bs];
  return mi_col * MI_SIZE + bw / 2 - 1;
}

static INLINE int block_center_y(int mi_row, BLOCK_SIZE bs) {
  const int bh = block_size_high[bs];
  return mi_row * MI_SIZE + bh / 2 - 1;
}

static INLINE int convert_to_trans_prec(int allow_hp, int coor) {
  const int shift =
      allow_hp ? WARPEDMODEL_PREC_BITS - 3 : WARPEDMODEL_PREC_BITS - 2;
  const int scale = allow_hp ? 0 : 1;
  return (ROUND_POWER_OF_TWO_SIGNED(coor, shift) << scale);
}

// Convert a global motion translation vector (which may have more bits than a
// regular motion vector) into a motion vector
static INLINE int_mv gm_get_motion_vector(const WarpedMotionParams *gm,
                                          int allow_hp, BLOCK_SIZE bsize,
                                          int mi_col, int mi_row,
                                          int block_idx) {
  const int unify_bsize = CONFIG_CB4X4;
  int_mv res;
  const int32_t *mat = gm->wmmat;
  int xc, yc, x, y;

  if (gm->wmtype == TRANSLATION) {
    res.as_mv.row = gm->wmmat[0] >> GM_TRANS_ONLY_PREC_DIFF;
    res.as_mv.col = gm->wmmat[1] >> GM_TRANS_ONLY_PREC_DIFF;
    return res;
  }

  if (bsize >= BLOCK_8X8 || unify_bsize) {
    x = block_center_x(mi_col, bsize);
    y = block_center_y(mi_row, bsize);
  } else {
    x = block_center_x(mi_col, bsize);
    y = block_center_y(mi_row, bsize);
    x += (block_idx & 1) * MI_SIZE / 2;
    y += (block_idx & 2) * MI_SIZE / 4;
  }

  if (gm->wmtype == ROTZOOM) {
    assert(gm->wmmat[5] == gm->wmmat[2]);
    assert(gm->wmmat[4] == -gm->wmmat[3]);
  }
  xc = mat[2] * x + mat[3] * y + mat[0];
  yc = mat[4] * x + mat[5] * y + mat[1];

  if (gm->wmtype > AFFINE) {
    const int Z =
        mat[6] * x + mat[7] * y + (1 << WARPEDMODEL_ROW3HOMO_PREC_BITS);
    xc <<= (WARPEDMODEL_ROW3HOMO_PREC_BITS - WARPEDMODEL_PREC_BITS);
    yc <<= (WARPEDMODEL_ROW3HOMO_PREC_BITS - WARPEDMODEL_PREC_BITS);
    xc = xc > 0 ? (xc + Z / 2) / Z : (xc - Z / 2) / Z;
    yc = yc > 0 ? (yc + Z / 2) / Z : (yc - Z / 2) / Z;
  }

  int tx = convert_to_trans_prec(allow_hp, xc) - (x << 3);
  int ty = convert_to_trans_prec(allow_hp, yc) - (y << 3);

  res.as_mv.row = ty;
  res.as_mv.col = tx;
  return res;
}

static INLINE TransformationType get_gmtype(const WarpedMotionParams *gm) {
  if (gm->wmmat[6] != 0 || gm->wmmat[7] != 0) {
    if (!gm->wmmat[6] && !gm->wmmat[4]) return HORTRAPEZOID;
    if (!gm->wmmat[7] && !gm->wmmat[3]) return VERTRAPEZOID;
    return HOMOGRAPHY;
  }
  if (gm->wmmat[5] == (1 << WARPEDMODEL_PREC_BITS) && !gm->wmmat[4] &&
      gm->wmmat[2] == (1 << WARPEDMODEL_PREC_BITS) && !gm->wmmat[3]) {
    return ((!gm->wmmat[1] && !gm->wmmat[0]) ? IDENTITY : TRANSLATION);
  }
  if (gm->wmmat[2] == gm->wmmat[5] && gm->wmmat[3] == -gm->wmmat[4])
    return ROTZOOM;
  else
    return AFFINE;
}

static INLINE void set_default_gmparams(WarpedMotionParams *wm) {
  static const int32_t default_wm_mat[8] = {
    0, 0, (1 << WARPEDMODEL_PREC_BITS), 0, 0, (1 << WARPEDMODEL_PREC_BITS), 0, 0
  };
  memcpy(wm->wmmat, default_wm_mat, sizeof(wm->wmmat));
  wm->wmtype = IDENTITY;
}
#endif  // CONFIG_GLOBAL_MOTION

#if CONFIG_REF_MV
typedef struct candidate_mv {
  int_mv this_mv;
  int_mv comp_mv;
  uint8_t pred_diff[2];
  int weight;
} CANDIDATE_MV;
#endif

static INLINE int is_zero_mv(const MV *mv) {
  return *((const uint32_t *)mv) == 0;
}

static INLINE int is_equal_mv(const MV *a, const MV *b) {
  return *((const uint32_t *)a) == *((const uint32_t *)b);
}

static INLINE void clamp_mv(MV *mv, int min_col, int max_col, int min_row,
                            int max_row) {
  mv->col = clamp(mv->col, min_col, max_col);
  mv->row = clamp(mv->row, min_row, max_row);
}

static INLINE int mv_has_subpel(const MV *mv) {
  return (mv->row & SUBPEL_MASK) || (mv->col & SUBPEL_MASK);
}
#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AV1_COMMON_MV_H_
