/*
 * Copyright (c) 2018, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#include <assert.h>
#include <smmintrin.h>

#include "./av1_rtcd.h"
#include "av1/common/convolve.h"
#include "av1/common/resize.h"
#include "aom_dsp/x86/synonyms.h"

// Note: If the crop width is not a multiple of 4, then, unlike the C version,
// this function will overwrite some of the padding on the right hand side of
// the frame. This padding appears to be trashed anyway, so this should not
// affect the running of the decoder.
void av1_convolve_horiz_rs_sse4_1(const uint8_t *src, int src_stride,
                                  uint8_t *dst, int dst_stride, int w, int h,
                                  const int16_t *x_filters, const int x0_qn,
                                  const int x_step_qn) {
  assert(UPSCALE_NORMATIVE_TAPS == 8);

  src -= UPSCALE_NORMATIVE_TAPS / 2 - 1;

  const __m128i round_add = _mm_set1_epi32((1 << FILTER_BITS) >> 1);
  const __m128i zero = _mm_setzero_si128();

  const uint8_t *src_y;
  uint8_t *dst_y;
  int x_qn = x0_qn;
  for (int x = 0; x < w; x += 4, x_qn += 4 * x_step_qn) {
    const int x_filter_idx0 =
        ((x_qn + 0 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;
    const int x_filter_idx1 =
        ((x_qn + 1 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;
    const int x_filter_idx2 =
        ((x_qn + 2 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;
    const int x_filter_idx3 =
        ((x_qn + 3 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;

    assert(x_filter_idx0 <= RS_SUBPEL_MASK);
    assert(x_filter_idx1 <= RS_SUBPEL_MASK);
    assert(x_filter_idx2 <= RS_SUBPEL_MASK);
    assert(x_filter_idx3 <= RS_SUBPEL_MASK);

    const int16_t *const x_filter0 =
        &x_filters[x_filter_idx0 * UPSCALE_NORMATIVE_TAPS];
    const int16_t *const x_filter1 =
        &x_filters[x_filter_idx1 * UPSCALE_NORMATIVE_TAPS];
    const int16_t *const x_filter2 =
        &x_filters[x_filter_idx2 * UPSCALE_NORMATIVE_TAPS];
    const int16_t *const x_filter3 =
        &x_filters[x_filter_idx3 * UPSCALE_NORMATIVE_TAPS];

    const __m128i fil0_16 = xx_loadu_128(x_filter0);
    const __m128i fil1_16 = xx_loadu_128(x_filter1);
    const __m128i fil2_16 = xx_loadu_128(x_filter2);
    const __m128i fil3_16 = xx_loadu_128(x_filter3);

    src_y = src;
    dst_y = dst;
    for (int y = 0; y < h; y++, src_y += src_stride, dst_y += dst_stride) {
      const uint8_t *const src_x0 =
          &src_y[(x_qn + 0 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];
      const uint8_t *const src_x1 =
          &src_y[(x_qn + 1 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];
      const uint8_t *const src_x2 =
          &src_y[(x_qn + 2 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];
      const uint8_t *const src_x3 =
          &src_y[(x_qn + 3 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];

      const __m128i src0_8 = xx_loadl_64(src_x0);
      const __m128i src1_8 = xx_loadl_64(src_x1);
      const __m128i src2_8 = xx_loadl_64(src_x2);
      const __m128i src3_8 = xx_loadl_64(src_x3);

      const __m128i src0_16 = _mm_cvtepu8_epi16(src0_8);
      const __m128i src1_16 = _mm_cvtepu8_epi16(src1_8);
      const __m128i src2_16 = _mm_cvtepu8_epi16(src2_8);
      const __m128i src3_16 = _mm_cvtepu8_epi16(src3_8);

      const __m128i conv0_32 = _mm_madd_epi16(src0_16, fil0_16);
      const __m128i conv1_32 = _mm_madd_epi16(src1_16, fil1_16);
      const __m128i conv2_32 = _mm_madd_epi16(src2_16, fil2_16);
      const __m128i conv3_32 = _mm_madd_epi16(src3_16, fil3_16);

      const __m128i conv01_32 = _mm_hadd_epi32(conv0_32, conv1_32);
      const __m128i conv23_32 = _mm_hadd_epi32(conv2_32, conv3_32);

      const __m128i conv0123_32 = _mm_hadd_epi32(conv01_32, conv23_32);

      const __m128i shifted_32 =
          _mm_srai_epi32(_mm_add_epi32(conv0123_32, round_add), FILTER_BITS);
      const __m128i shifted_16 = _mm_packus_epi32(shifted_32, zero);
      const __m128i shifted_8 = _mm_packus_epi16(shifted_16, zero);

      xx_storel_32(&dst_y[x], shifted_8);
    }
  }
}

// Note: If the crop width is not a multiple of 4, then, unlike the C version,
// this function will overwrite some of the padding on the right hand side of
// the frame. This padding appears to be trashed anyway, so this should not
// affect the running of the decoder.
void av1_highbd_convolve_horiz_rs_sse4_1(const uint16_t *src, int src_stride,
                                         uint16_t *dst, int dst_stride, int w,
                                         int h, const int16_t *x_filters,
                                         const int x0_qn, const int x_step_qn,
                                         int bd) {
  assert(UPSCALE_NORMATIVE_TAPS == 8);
  assert(bd == 8 || bd == 10 || bd == 12);

  src -= UPSCALE_NORMATIVE_TAPS / 2 - 1;

  const __m128i round_add = _mm_set1_epi32((1 << FILTER_BITS) >> 1);
  const __m128i zero = _mm_setzero_si128();
  const __m128i clip_maximum = _mm_set1_epi16((1 << bd) - 1);

  const uint16_t *src_y;
  uint16_t *dst_y;
  int x_qn = x0_qn;
  for (int x = 0; x < w; x += 4, x_qn += 4 * x_step_qn) {
    const int x_filter_idx0 =
        ((x_qn + 0 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;
    const int x_filter_idx1 =
        ((x_qn + 1 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;
    const int x_filter_idx2 =
        ((x_qn + 2 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;
    const int x_filter_idx3 =
        ((x_qn + 3 * x_step_qn) & RS_SCALE_SUBPEL_MASK) >> RS_SCALE_EXTRA_BITS;

    assert(x_filter_idx0 <= RS_SUBPEL_MASK);
    assert(x_filter_idx1 <= RS_SUBPEL_MASK);
    assert(x_filter_idx2 <= RS_SUBPEL_MASK);
    assert(x_filter_idx3 <= RS_SUBPEL_MASK);

    const int16_t *const x_filter0 =
        &x_filters[x_filter_idx0 * UPSCALE_NORMATIVE_TAPS];
    const int16_t *const x_filter1 =
        &x_filters[x_filter_idx1 * UPSCALE_NORMATIVE_TAPS];
    const int16_t *const x_filter2 =
        &x_filters[x_filter_idx2 * UPSCALE_NORMATIVE_TAPS];
    const int16_t *const x_filter3 =
        &x_filters[x_filter_idx3 * UPSCALE_NORMATIVE_TAPS];

    const __m128i fil0_16 = xx_loadu_128(x_filter0);
    const __m128i fil1_16 = xx_loadu_128(x_filter1);
    const __m128i fil2_16 = xx_loadu_128(x_filter2);
    const __m128i fil3_16 = xx_loadu_128(x_filter3);

    src_y = src;
    dst_y = dst;
    for (int y = 0; y < h; y++, src_y += src_stride, dst_y += dst_stride) {
      const uint16_t *const src_x0 =
          &src_y[(x_qn + 0 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];
      const uint16_t *const src_x1 =
          &src_y[(x_qn + 1 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];
      const uint16_t *const src_x2 =
          &src_y[(x_qn + 2 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];
      const uint16_t *const src_x3 =
          &src_y[(x_qn + 3 * x_step_qn) >> RS_SCALE_SUBPEL_BITS];

      const __m128i src0_16 = xx_loadu_128(src_x0);
      const __m128i src1_16 = xx_loadu_128(src_x1);
      const __m128i src2_16 = xx_loadu_128(src_x2);
      const __m128i src3_16 = xx_loadu_128(src_x3);

      const __m128i conv0_32 = _mm_madd_epi16(src0_16, fil0_16);
      const __m128i conv1_32 = _mm_madd_epi16(src1_16, fil1_16);
      const __m128i conv2_32 = _mm_madd_epi16(src2_16, fil2_16);
      const __m128i conv3_32 = _mm_madd_epi16(src3_16, fil3_16);

      const __m128i conv01_32 = _mm_hadd_epi32(conv0_32, conv1_32);
      const __m128i conv23_32 = _mm_hadd_epi32(conv2_32, conv3_32);

      const __m128i conv0123_32 = _mm_hadd_epi32(conv01_32, conv23_32);

      const __m128i shifted_32 =
          _mm_srai_epi32(_mm_add_epi32(conv0123_32, round_add), FILTER_BITS);
      const __m128i shifted_16 = _mm_packus_epi32(shifted_32, zero);

      const __m128i clipped_16 = _mm_min_epi16(shifted_16, clip_maximum);

      xx_storel_64(&dst_y[x], clipped_16);
    }
  }
}
