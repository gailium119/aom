/*
 * Copyright (c) 2001-2016, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

/* clang-format off */

#ifndef AV1_COMMON_ODINTRIN_H_
#define AV1_COMMON_ODINTRIN_H_

#include <stdlib.h>
#include <string.h>

#include "aom/aom_integer.h"
#include "aom_dsp/aom_dsp_common.h"
#include "aom_ports/bitops.h"
#include "av1/common/enums.h"

#ifdef __cplusplus
extern "C" {
#endif

# if !defined(M_PI)
#  define M_PI      (3.1415926535897932384626433832795)
# endif

# if !defined(M_SQRT2)
#  define M_SQRT2 (1.41421356237309504880168872420970)
# endif

# if !defined(M_SQRT1_2)
#  define M_SQRT1_2 (0.70710678118654752440084436210485)
# endif

# if !defined(M_LOG2E)
#  define M_LOG2E (1.4426950408889634073599246810019)
# endif

# if !defined(M_LN2)
#  define M_LN2 (0.69314718055994530941723212145818)
# endif

typedef int od_coeff;

#define OD_DIVU_DMAX (1024)

#define OD_MINI AOMMIN
#define OD_MAXI AOMMAX
#define OD_CLAMPI(min, val, max) (OD_MAXI(min, OD_MINI(val, max)))

#define OD_CLZ0 (1)
#define OD_CLZ(x) (-get_msb(x))
#define OD_ILOG_NZ(x) (OD_CLZ0 - OD_CLZ(x))

#define OD_LOG2(x) (M_LOG2E*log(x))
#define OD_EXP2(x) (exp(M_LN2*(x)))

/*Enable special features for gcc and compatible compilers.*/
#if defined(__GNUC__) && defined(__GNUC_MINOR__) && defined(__GNUC_PATCHLEVEL__)
#define OD_GNUC_PREREQ(maj, min, pat)                                \
  ((__GNUC__ << 16) + (__GNUC_MINOR__ << 8) + __GNUC_PATCHLEVEL__ >= \
   ((maj) << 16) + ((min) << 8) + pat)  // NOLINT
#else
#define OD_GNUC_PREREQ(maj, min, pat) (0)
#endif

#if OD_GNUC_PREREQ(3, 4, 0)
#define OD_WARN_UNUSED_RESULT __attribute__((__warn_unused_result__))
#else
#define OD_WARN_UNUSED_RESULT
#endif

#if OD_GNUC_PREREQ(3, 4, 0)
#define OD_ARG_NONNULL(x) __attribute__((__nonnull__(x)))
#else
#define OD_ARG_NONNULL(x)
#endif

#define OD_ASSERT(_cond) assert(_cond)

/** Copy n elements of memory from src to dst. The 0* term provides
    compile-time type checking  */
#if !defined(OVERRIDE_OD_COPY)
#define OD_COPY(dst, src, n) \
  (memcpy((dst), (src), sizeof(*(dst)) * (n) + 0 * ((dst) - (src))))
#endif

/** Copy n elements of memory from src to dst, allowing overlapping regions.
    The 0* term provides compile-time type checking */
#if !defined(OVERRIDE_OD_MOVE)
# define OD_MOVE(dst, src, n) \
 (memmove((dst), (src), sizeof(*(dst))*(n) + 0*((dst) - (src)) ))
#endif

/*All of these macros should expect floats as arguments.*/
# define OD_SIGNMASK(a) (-((a) < 0))
# define OD_FLIPSIGNI(a, b) (((a) + OD_SIGNMASK(b)) ^ OD_SIGNMASK(b))

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // AV1_COMMON_ODINTRIN_H_
