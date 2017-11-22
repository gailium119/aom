/*
 * Copyright (c) 2017, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_DSP_X86_MEM_SSE2_H_
#define AOM_DSP_X86_MEM_SSE2_H_

#include <emmintrin.h>  // SSE2

#include "./aom_config.h"
#include "aom/aom_integer.h"

static INLINE __m128i loadh_epi64(const void *const src, const __m128i s) {
  return _mm_castps_si128(
      _mm_loadh_pi(_mm_castsi128_ps(s), (const __m64 *)src));
}

static INLINE __m128i load_8bit_4x2_to_1_sse2(const uint8_t *const src,
                                              const ptrdiff_t stride) {
  const __m128i s0 = _mm_cvtsi32_si128(*(const int *)(src + 0 * stride));
  const __m128i s1 = _mm_cvtsi32_si128(*(const int *)(src + 1 * stride));
  return _mm_unpacklo_epi32(s0, s1);
}

#endif  // AOM_DSP_X86_MEM_SSE2_H_
