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

#ifndef AOM_AV1_COMMON_FILTER_H_
#define AOM_AV1_COMMON_FILTER_H_

#include <assert.h>

#include "config/aom_config.h"

#include "aom/aom_integer.h"
#include "aom_dsp/aom_filter.h"
#include "aom_ports/mem.h"
#include "av1/common/enums.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_FILTER_TAP 8

typedef enum ATTRIBUTE_PACKED {
  EIGHTTAP_REGULAR,
  EIGHTTAP_SMOOTH,
  MULTITAP_SHARP,
  BILINEAR,
  // Encoder side only filters
  MULTITAP_SHARP2,

  INTERP_FILTERS_ALL,
  SWITCHABLE_FILTERS = BILINEAR,
  SWITCHABLE = SWITCHABLE_FILTERS + 1, /* the last switchable one */
  EXTRA_FILTERS = INTERP_FILTERS_ALL - SWITCHABLE_FILTERS,
  INTERP_INVALID = 0xff,
} InterpFilter;

enum {
  FILTER_UNUSED = 0,  // No longer used
  USE_2_TAPS,
  USE_4_TAPS,
  USE_8_TAPS,
} UENUM1BYTE(SUBPEL_SEARCH_TYPE);

enum {
  INTERP_EVAL_LUMA_EVAL_CHROMA = 0,
  INTERP_SKIP_LUMA_EVAL_CHROMA,
  INTERP_EVAL_INVALID,
  INTERP_SKIP_LUMA_SKIP_CHROMA,
} UENUM1BYTE(INTERP_EVAL_PLANE);

#if !CONFIG_REMOVE_DUAL_FILTER
enum {
  INTERP_HORZ_NEQ_VERT_NEQ = 0,
  INTERP_HORZ_EQ_VERT_NEQ,
  INTERP_HORZ_NEQ_VERT_EQ,
  INTERP_HORZ_EQ_VERT_EQ,
  INTERP_PRED_TYPE_ALL,
} UENUM1BYTE(INTERP_PRED_TYPE);

static const uint16_t
    av1_interp_dual_filt_mask[INTERP_PRED_TYPE_ALL - 2][SWITCHABLE_FILTERS] = {
      { (1 << REG_REG) | (1 << SMOOTH_REG) | (1 << SHARP_REG),
        (1 << REG_SMOOTH) | (1 << SMOOTH_SMOOTH) | (1 << SHARP_SMOOTH),
        (1 << REG_SHARP) | (1 << SMOOTH_SHARP) | (1 << SHARP_SHARP) },
      { (1 << REG_REG) | (1 << REG_SMOOTH) | (1 << REG_SHARP),
        (1 << SMOOTH_REG) | (1 << SMOOTH_SMOOTH) | (1 << SMOOTH_SHARP),
        (1 << SHARP_REG) | (1 << SHARP_SMOOTH) | (1 << SHARP_SHARP) }
    };

// Pack two InterpFilter's into a uint32_t: since there are at most 10 filters,
// we can use 16 bits for each and have more than enough space. This reduces
// argument passing and unifies the operation of setting a (pair of) filters.
typedef struct InterpFilters {
  uint16_t y_filter;
  uint16_t x_filter;
} InterpFilters;

typedef union int_interpfilters {
  uint32_t as_int;
  InterpFilters as_filters;
} int_interpfilters;

static INLINE InterpFilter av1_extract_interp_filter(int_interpfilters filters,
                                                     int dir) {
  return (InterpFilter)((dir) ? filters.as_filters.x_filter
                              : filters.as_filters.y_filter);
}

static INLINE int_interpfilters
av1_broadcast_interp_filter(InterpFilter filter) {
  int_interpfilters filters;
  filters.as_filters.x_filter = filter;
  filters.as_filters.y_filter = filter;
  return filters;
}
#endif  // !CONFIG_REMOVE_DUAL_FILTER

static INLINE InterpFilter av1_unswitchable_filter(InterpFilter filter) {
  return filter == SWITCHABLE ? EIGHTTAP_REGULAR : filter;
}

/* (1 << LOG_SWITCHABLE_FILTERS) > SWITCHABLE_FILTERS */
#define LOG_SWITCHABLE_FILTERS 2

#define SWITCHABLE_FILTER_CONTEXTS ((SWITCHABLE_FILTERS + 1) * 4)
#define INTER_FILTER_COMP_OFFSET (SWITCHABLE_FILTERS + 1)
#define INTER_FILTER_DIR_OFFSET ((SWITCHABLE_FILTERS + 1) * 2)
#define ALLOW_ALL_INTERP_FILT_MASK (0x01ff)

typedef struct InterpFilterParams {
  const int16_t *filter_ptr;
  uint16_t taps;
  InterpFilter interp_filter;
} InterpFilterParams;

DECLARE_ALIGNED(256, static const InterpKernel,
                av1_bilinear_filters[SUBPEL_SHIFTS]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },  { 0, 0, 0, 120, 8, 0, 0, 0 },
  { 0, 0, 0, 112, 16, 0, 0, 0 }, { 0, 0, 0, 104, 24, 0, 0, 0 },
  { 0, 0, 0, 96, 32, 0, 0, 0 },  { 0, 0, 0, 88, 40, 0, 0, 0 },
  { 0, 0, 0, 80, 48, 0, 0, 0 },  { 0, 0, 0, 72, 56, 0, 0, 0 },
  { 0, 0, 0, 64, 64, 0, 0, 0 },  { 0, 0, 0, 56, 72, 0, 0, 0 },
  { 0, 0, 0, 48, 80, 0, 0, 0 },  { 0, 0, 0, 40, 88, 0, 0, 0 },
  { 0, 0, 0, 32, 96, 0, 0, 0 },  { 0, 0, 0, 24, 104, 0, 0, 0 },
  { 0, 0, 0, 16, 112, 0, 0, 0 }, { 0, 0, 0, 8, 120, 0, 0, 0 }
};

DECLARE_ALIGNED(256, static const InterpKernel,
                av1_sub_pel_filters_8[SUBPEL_SHIFTS]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },      { 0, 2, -6, 126, 8, -2, 0, 0 },
  { 0, 2, -10, 122, 18, -4, 0, 0 },  { 0, 2, -12, 116, 28, -8, 2, 0 },
  { 0, 2, -14, 110, 38, -10, 2, 0 }, { 0, 2, -14, 102, 48, -12, 2, 0 },
  { 0, 2, -16, 94, 58, -12, 2, 0 },  { 0, 2, -14, 84, 66, -12, 2, 0 },
  { 0, 2, -14, 76, 76, -14, 2, 0 },  { 0, 2, -12, 66, 84, -14, 2, 0 },
  { 0, 2, -12, 58, 94, -16, 2, 0 },  { 0, 2, -12, 48, 102, -14, 2, 0 },
  { 0, 2, -10, 38, 110, -14, 2, 0 }, { 0, 2, -8, 28, 116, -12, 2, 0 },
  { 0, 0, -4, 18, 122, -10, 2, 0 },  { 0, 0, -2, 8, 126, -6, 2, 0 }
};

DECLARE_ALIGNED(256, static const InterpKernel,
                av1_sub_pel_filters_8sharp[SUBPEL_SHIFTS]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },         { -2, 2, -6, 126, 8, -2, 2, 0 },
  { -2, 6, -12, 124, 16, -6, 4, -2 },   { -2, 8, -18, 120, 26, -10, 6, -2 },
  { -4, 10, -22, 116, 38, -14, 6, -2 }, { -4, 10, -22, 108, 48, -18, 8, -2 },
  { -4, 10, -24, 100, 60, -20, 8, -2 }, { -4, 10, -24, 90, 70, -22, 10, -2 },
  { -4, 12, -24, 80, 80, -24, 12, -4 }, { -2, 10, -22, 70, 90, -24, 10, -4 },
  { -2, 8, -20, 60, 100, -24, 10, -4 }, { -2, 8, -18, 48, 108, -22, 10, -4 },
  { -2, 6, -14, 38, 116, -22, 10, -4 }, { -2, 6, -10, 26, 120, -18, 8, -2 },
  { -2, 4, -6, 16, 124, -12, 6, -2 },   { 0, 2, -2, 8, 126, -6, 2, -2 }
};

DECLARE_ALIGNED(256, static const InterpKernel,
                av1_sub_pel_filters_8smooth[SUBPEL_SHIFTS]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },     { 0, 2, 28, 62, 34, 2, 0, 0 },
  { 0, 0, 26, 62, 36, 4, 0, 0 },    { 0, 0, 22, 62, 40, 4, 0, 0 },
  { 0, 0, 20, 60, 42, 6, 0, 0 },    { 0, 0, 18, 58, 44, 8, 0, 0 },
  { 0, 0, 16, 56, 46, 10, 0, 0 },   { 0, -2, 16, 54, 48, 12, 0, 0 },
  { 0, -2, 14, 52, 52, 14, -2, 0 }, { 0, 0, 12, 48, 54, 16, -2, 0 },
  { 0, 0, 10, 46, 56, 16, 0, 0 },   { 0, 0, 8, 44, 58, 18, 0, 0 },
  { 0, 0, 6, 42, 60, 20, 0, 0 },    { 0, 0, 4, 40, 62, 22, 0, 0 },
  { 0, 0, 4, 36, 62, 26, 0, 0 },    { 0, 0, 2, 34, 62, 28, 2, 0 }
};

DECLARE_ALIGNED(256, static const int16_t,
                av1_sub_pel_filters_12sharp[SUBPEL_SHIFTS][12]) = {
  { 0, 0, 0, 0, 0, 128, 0, 0, 0, 0, 0, 0 },
  { 0, 1, -2, 3, -7, 127, 8, -4, 2, -1, 1, 0 },
  { -1, 2, -3, 6, -13, 124, 18, -8, 4, -2, 2, -1 },
  { -1, 3, -4, 8, -18, 120, 28, -12, 7, -4, 2, -1 },
  { -1, 3, -6, 10, -21, 115, 38, -15, 8, -5, 3, -1 },
  { -2, 4, -6, 12, -24, 108, 49, -18, 10, -6, 3, -2 },
  { -2, 4, -7, 13, -25, 100, 60, -21, 11, -7, 4, -2 },
  { -2, 4, -7, 13, -26, 91, 71, -24, 13, -7, 4, -2 },
  { -2, 4, -7, 13, -25, 81, 81, -25, 13, -7, 4, -2 },
  { -2, 4, -7, 13, -24, 71, 91, -26, 13, -7, 4, -2 },
  { -2, 4, -7, 11, -21, 60, 100, -25, 13, -7, 4, -2 },
  { -2, 3, -6, 10, -18, 49, 108, -24, 12, -6, 4, -2 },
  { -1, 3, -5, 8, -15, 38, 115, -21, 10, -6, 3, -1 },
  { -1, 2, -4, 7, -12, 28, 120, -18, 8, -4, 3, -1 },
  { -1, 2, -2, 4, -8, 18, 124, -13, 6, -3, 2, -1 },
  { 0, 1, -1, 2, -4, 8, 127, -7, 3, -2, 1, 0 }
};

#if CONFIG_OPTFLOW_REFINEMENT
// TODO(kslu): play with the sharpness
DECLARE_ALIGNED(256, static const InterpKernel,
                av1_subpel32_filters_8sharp[32]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },         { 0, 2, -4, 128, 4, -2, 0, 0 },
  { 0, 2, -6, 126, 8, -4, 2, 0 },       { -2, 4, -8, 126, 12, -6, 2, 0 },
  { -2, 4, -12, 124, 18, -6, 4, -2 },   { -2, 6, -14, 122, 22, -8, 4, -2 },
  { -2, 6, -16, 120, 28, -10, 4, -2 },  { -2, 6, -18, 116, 34, -12, 6, -2 },
  { -2, 6, -18, 114, 38, -14, 6, -2 },  { -2, 8, -20, 110, 44, -16, 6, -2 },
  { -2, 8, -22, 106, 48, -16, 8, -2 },  { -2, 8, -22, 102, 54, -18, 8, -2 },
  { -2, 8, -22, 98, 60, -20, 8, -2 },   { -4, 10, -22, 94, 64, -20, 8, -2 },
  { -4, 10, -22, 90, 70, -20, 8, -4 },  { -4, 10, -22, 84, 74, -22, 10, -2 },
  { -4, 10, -22, 80, 80, -22, 10, -4 }, { -2, 10, -22, 74, 84, -22, 10, -4 },
  { -4, 8, -20, 70, 90, -22, 10, -4 },  { -2, 8, -20, 64, 94, -22, 10, -4 },
  { -2, 8, -20, 60, 98, -22, 8, -2 },   { -2, 8, -18, 54, 102, -22, 8, -2 },
  { -2, 8, -16, 48, 106, -22, 8, -2 },  { -2, 6, -16, 44, 110, -20, 8, -2 },
  { -2, 6, -14, 38, 114, -18, 6, -2 },  { -2, 6, -12, 34, 116, -18, 6, -2 },
  { -2, 4, -10, 28, 120, -16, 6, -2 },  { -2, 4, -8, 22, 122, -14, 6, -2 },
  { -2, 4, -6, 18, 124, -12, 4, -2 },   { 0, 2, -6, 12, 126, -8, 4, -2 },
  { 0, 2, -4, 8, 126, -6, 2, 0 },       { 0, 0, -2, 4, 128, -4, 2, 0 },
};

DECLARE_ALIGNED(256, static const InterpKernel,
                av1_subpel64_filters_8sharp[64]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },         { 0, 0, -2, 128, 2, 0, 0, 0 },
  { 0, 2, -4, 128, 4, -2, 0, 0 },       { 0, 2, -4, 126, 6, -2, 0, 0 },
  { 0, 2, -6, 126, 8, -4, 2, 0 },       { 0, 2, -8, 126, 10, -4, 2, 0 },
  { -2, 4, -8, 126, 12, -6, 2, 0 },     { -2, 4, -10, 124, 16, -6, 2, 0 },
  { -2, 4, -12, 124, 18, -6, 4, -2 },   { -2, 4, -12, 122, 20, -8, 4, 0 },
  { -2, 6, -14, 122, 22, -8, 4, -2 },   { -2, 6, -14, 120, 26, -10, 4, -2 },
  { -2, 6, -16, 120, 28, -10, 4, -2 },  { -2, 6, -16, 118, 30, -12, 6, -2 },
  { -2, 6, -18, 116, 34, -12, 6, -2 },  { -2, 6, -18, 114, 36, -12, 6, -2 },
  { -2, 6, -18, 114, 38, -14, 6, -2 },  { -2, 8, -20, 112, 40, -14, 6, -2 },
  { -2, 8, -20, 110, 44, -16, 6, -2 },  { -2, 8, -20, 108, 46, -16, 6, -2 },
  { -2, 8, -22, 106, 48, -16, 8, -2 },  { -2, 8, -22, 104, 52, -18, 8, -2 },
  { -2, 8, -22, 102, 54, -18, 8, -2 },  { -2, 8, -22, 100, 56, -18, 8, -2 },
  { -2, 8, -22, 98, 60, -20, 8, -2 },   { -2, 8, -22, 96, 62, -20, 8, -2 },
  { -4, 10, -22, 94, 64, -20, 8, -2 },  { -4, 10, -22, 92, 66, -20, 8, -2 },
  { -4, 10, -22, 90, 70, -20, 8, -4 },  { -4, 10, -22, 88, 72, -22, 8, -2 },
  { -4, 10, -22, 84, 74, -22, 10, -2 }, { -4, 10, -22, 82, 78, -22, 10, -4 },
  { -4, 10, -22, 80, 80, -22, 10, -4 }, { -4, 10, -22, 78, 82, -22, 10, -4 },
  { -2, 10, -22, 74, 84, -22, 10, -4 }, { -2, 8, -22, 72, 88, -22, 10, -4 },
  { -4, 8, -20, 70, 90, -22, 10, -4 },  { -2, 8, -20, 66, 92, -22, 10, -4 },
  { -2, 8, -20, 64, 94, -22, 10, -4 },  { -2, 8, -20, 62, 96, -22, 8, -2 },
  { -2, 8, -20, 60, 98, -22, 8, -2 },   { -2, 8, -18, 56, 100, -22, 8, -2 },
  { -2, 8, -18, 54, 102, -22, 8, -2 },  { -2, 8, -18, 52, 104, -22, 8, -2 },
  { -2, 8, -16, 48, 106, -22, 8, -2 },  { -2, 6, -16, 46, 108, -20, 8, -2 },
  { -2, 6, -16, 44, 110, -20, 8, -2 },  { -2, 6, -14, 40, 112, -20, 8, -2 },
  { -2, 6, -14, 38, 114, -18, 6, -2 },  { -2, 6, -12, 36, 114, -18, 6, -2 },
  { -2, 6, -12, 34, 116, -18, 6, -2 },  { -2, 6, -12, 30, 118, -16, 6, -2 },
  { -2, 4, -10, 28, 120, -16, 6, -2 },  { -2, 4, -10, 26, 120, -14, 6, -2 },
  { -2, 4, -8, 22, 122, -14, 6, -2 },   { 0, 4, -8, 20, 122, -12, 4, -2 },
  { -2, 4, -6, 18, 124, -12, 4, -2 },   { 0, 2, -6, 16, 124, -10, 4, -2 },
  { 0, 2, -6, 12, 126, -8, 4, -2 },     { 0, 2, -4, 10, 126, -8, 2, 0 },
  { 0, 2, -4, 8, 126, -6, 2, 0 },       { 0, 0, -2, 6, 126, -4, 2, 0 },
  { 0, 0, -2, 4, 128, -4, 2, 0 },       { 0, 0, 0, 2, 128, -2, 0, 0 },
};

static const InterpFilterParams av1_opfl_interp_filter_params_list[3] = {
  { (const int16_t *)av1_sub_pel_filters_8sharp, SUBPEL_TAPS, MULTITAP_SHARP },
  { (const int16_t *)av1_subpel32_filters_8sharp, SUBPEL_TAPS, MULTITAP_SHARP },
  { (const int16_t *)av1_subpel64_filters_8sharp, SUBPEL_TAPS, MULTITAP_SHARP },
};
#endif  // CONFIG_OPTFLOW_REFINEMENT

static const InterpFilterParams
    av1_interp_filter_params_list[INTERP_FILTERS_ALL] = {
      { (const int16_t *)av1_sub_pel_filters_8, SUBPEL_TAPS, EIGHTTAP_REGULAR },
      { (const int16_t *)av1_sub_pel_filters_8smooth, SUBPEL_TAPS,
        EIGHTTAP_SMOOTH },
      { (const int16_t *)av1_sub_pel_filters_8sharp, SUBPEL_TAPS,
        MULTITAP_SHARP },
      { (const int16_t *)av1_bilinear_filters, SUBPEL_TAPS, BILINEAR },

      // The following filters are for encoder only, and now they are used in
      // temporal filtering. The predictor block size >= 16 in temporal filter.
      { (const int16_t *)av1_sub_pel_filters_12sharp, 12, MULTITAP_SHARP2 },
    };

// A special 2-tap bilinear filter for IntraBC chroma. IntraBC uses full pixel
// MV for luma. If sub-sampling exists, chroma may possibly use half-pel MV.
DECLARE_ALIGNED(256, static const int16_t,
                av1_intrabc_bilinear_filter[2 * SUBPEL_SHIFTS]) = {
  128, 0,  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  64,  64, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
};

static const InterpFilterParams av1_intrabc_filter_params = {
  av1_intrabc_bilinear_filter, 2, BILINEAR
};

DECLARE_ALIGNED(256, static const InterpKernel,
                av1_sub_pel_filters_4[SUBPEL_SHIFTS]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },     { 0, 0, -4, 126, 8, -2, 0, 0 },
  { 0, 0, -8, 122, 18, -4, 0, 0 },  { 0, 0, -10, 116, 28, -6, 0, 0 },
  { 0, 0, -12, 110, 38, -8, 0, 0 }, { 0, 0, -12, 102, 48, -10, 0, 0 },
  { 0, 0, -14, 94, 58, -10, 0, 0 }, { 0, 0, -12, 84, 66, -10, 0, 0 },
  { 0, 0, -12, 76, 76, -12, 0, 0 }, { 0, 0, -10, 66, 84, -12, 0, 0 },
  { 0, 0, -10, 58, 94, -14, 0, 0 }, { 0, 0, -10, 48, 102, -12, 0, 0 },
  { 0, 0, -8, 38, 110, -12, 0, 0 }, { 0, 0, -6, 28, 116, -10, 0, 0 },
  { 0, 0, -4, 18, 122, -8, 0, 0 },  { 0, 0, -2, 8, 126, -4, 0, 0 }
};
DECLARE_ALIGNED(256, static const InterpKernel,
                av1_sub_pel_filters_4smooth[SUBPEL_SHIFTS]) = {
  { 0, 0, 0, 128, 0, 0, 0, 0 },   { 0, 0, 30, 62, 34, 2, 0, 0 },
  { 0, 0, 26, 62, 36, 4, 0, 0 },  { 0, 0, 22, 62, 40, 4, 0, 0 },
  { 0, 0, 20, 60, 42, 6, 0, 0 },  { 0, 0, 18, 58, 44, 8, 0, 0 },
  { 0, 0, 16, 56, 46, 10, 0, 0 }, { 0, 0, 14, 54, 48, 12, 0, 0 },
  { 0, 0, 12, 52, 52, 12, 0, 0 }, { 0, 0, 12, 48, 54, 14, 0, 0 },
  { 0, 0, 10, 46, 56, 16, 0, 0 }, { 0, 0, 8, 44, 58, 18, 0, 0 },
  { 0, 0, 6, 42, 60, 20, 0, 0 },  { 0, 0, 4, 40, 62, 22, 0, 0 },
  { 0, 0, 4, 36, 62, 26, 0, 0 },  { 0, 0, 2, 34, 62, 30, 0, 0 }
};

// For w<=4, MULTITAP_SHARP is the same as EIGHTTAP_REGULAR
static const InterpFilterParams av1_interp_4tap[SWITCHABLE_FILTERS + 1] = {
  { (const int16_t *)av1_sub_pel_filters_4, SUBPEL_TAPS, EIGHTTAP_REGULAR },
  { (const int16_t *)av1_sub_pel_filters_4smooth, SUBPEL_TAPS,
    EIGHTTAP_SMOOTH },
  { (const int16_t *)av1_sub_pel_filters_4, SUBPEL_TAPS, EIGHTTAP_REGULAR },
  { (const int16_t *)av1_bilinear_filters, SUBPEL_TAPS, BILINEAR },
};

static INLINE const InterpFilterParams *
av1_get_interp_filter_params_with_block_size(const InterpFilter interp_filter,
                                             const int w) {
  if (w <= 4) return &av1_interp_4tap[interp_filter];
  return &av1_interp_filter_params_list[interp_filter];
}

static INLINE const int16_t *av1_get_interp_filter_kernel(
    const InterpFilter interp_filter, int subpel_search) {
  assert(subpel_search >= USE_2_TAPS);
  return (subpel_search == USE_2_TAPS)
             ? av1_interp_4tap[BILINEAR].filter_ptr
             : ((subpel_search == USE_4_TAPS)
                    ? av1_interp_4tap[interp_filter].filter_ptr
                    : av1_interp_filter_params_list[interp_filter].filter_ptr);
}

static INLINE const int16_t *av1_get_interp_filter_subpel_kernel(
    const InterpFilterParams *const filter_params, const int subpel) {
  return filter_params->filter_ptr + filter_params->taps * subpel;
}

static INLINE const InterpFilterParams *av1_get_filter(int subpel_search) {
  assert(subpel_search >= USE_2_TAPS);

  switch (subpel_search) {
    case USE_2_TAPS: return &av1_interp_4tap[BILINEAR];
    case USE_4_TAPS: return &av1_interp_4tap[EIGHTTAP_REGULAR];
    case USE_8_TAPS: return &av1_interp_filter_params_list[EIGHTTAP_REGULAR];
    default: assert(0); return NULL;
  }
}

#if !CONFIG_REMOVE_DUAL_FILTER
static INLINE void reset_interp_filter_allowed_mask(
    uint16_t *allow_interp_mask, DUAL_FILTER_TYPE filt_type) {
  uint16_t tmp = (~(1 << filt_type)) & 0xffff;
  *allow_interp_mask &= (tmp & ALLOW_ALL_INTERP_FILT_MASK);
}

static INLINE void set_interp_filter_allowed_mask(uint16_t *allow_interp_mask,
                                                  DUAL_FILTER_TYPE filt_type) {
  *allow_interp_mask |= (1 << filt_type);
}

static INLINE uint8_t get_interp_filter_allowed_mask(
    uint16_t allow_interp_mask, DUAL_FILTER_TYPE filt_type) {
  return (allow_interp_mask >> filt_type) & 1;
}
#endif  // !CONFIG_REMOVE_DUAL_FILTER

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AOM_AV1_COMMON_FILTER_H_
