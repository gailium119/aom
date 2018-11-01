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

#include <emmintrin.h>

#include "config/aom_dsp_rtcd.h"

#include "aom_dsp/aom_filter.h"
#include "aom_dsp/x86/convolve_sse2.h"

void av1_jnt_convolve_x_sse2(const uint8_t *src, int src_stride, uint8_t *dst0,
                             int dst_stride0, int w, int h,
                             const InterpFilterParams *filter_params_x,
                             const InterpFilterParams *filter_params_y,
                             const int subpel_x_q4, const int subpel_y_q4,
                             ConvolveParams *conv_params) {
  const int bd = 8;
  CONV_BUF_TYPE *dst = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  const int fo_horiz = filter_params_x->taps / 2 - 1;
  const uint8_t *src_ptr = src - fo_horiz;
  const int bits = FILTER_BITS - conv_params->round_1;
  const __m128i left_shift = _mm_cvtsi32_si128(bits);
  const __m128i round_const = _mm_set1_epi32((1 << conv_params->round_0) >> 1);
  const __m128i round_shift = _mm_cvtsi32_si128(conv_params->round_0);
  const int w0 = conv_params->fwd_offset;
  const int w1 = conv_params->bck_offset;
  const __m128i wt0 = _mm_set1_epi16(w0);
  const __m128i wt1 = _mm_set1_epi16(w1);
  const __m128i wt = _mm_unpacklo_epi16(wt0, wt1);
  const int do_average = conv_params->do_average;
  const int use_jnt_comp_avg = conv_params->use_jnt_comp_avg;
  const int offset_0 =
      bd + 2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  const int offset = (1 << offset_0) + (1 << (offset_0 - 1));
  const __m128i offset_const = _mm_set1_epi16(offset);
  const int rounding_shift =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  const __m128i rounding_const = _mm_set1_epi16((1 << rounding_shift) >> 1);
  __m128i coeffs[4];

  (void)filter_params_y;
  (void)subpel_y_q4;

  prepare_coeffs(filter_params_x, subpel_x_q4, coeffs);

  if (w == 4) {
    do {
      const __m128i data = _mm_loadu_si128((__m128i *)src_ptr);
      __m128i s[4];

      s[0] = _mm_unpacklo_epi8(data, _mm_srli_si128(data, 1));
      s[1] =
          _mm_unpacklo_epi8(_mm_srli_si128(data, 2), _mm_srli_si128(data, 3));
      s[2] =
          _mm_unpacklo_epi8(_mm_srli_si128(data, 4), _mm_srli_si128(data, 5));
      s[3] =
          _mm_unpacklo_epi8(_mm_srli_si128(data, 6), _mm_srli_si128(data, 7));
      const __m128i res_lo = convolve_lo_x(s, coeffs);
      const __m128i res_lo_round =
          _mm_sra_epi32(_mm_add_epi32(res_lo, round_const), round_shift);
      const __m128i res_lo_shift = _mm_sll_epi32(res_lo_round, left_shift);

      const __m128i res_16b = _mm_packs_epi32(res_lo_shift, res_lo_shift);
      const __m128i res_unsigned = _mm_add_epi16(res_16b, offset_const);

      // Accumulate values into the destination buffer
      if (do_average) {
        const __m128i data_ref_0 = _mm_loadu_si128((__m128i *)dst);

        const __m128i comp_avg_res =
            comp_avg(&data_ref_0, &res_unsigned, &wt, use_jnt_comp_avg);

        const __m128i round_result = convolve_rounding(
            &comp_avg_res, &offset_const, &rounding_const, rounding_shift);

        const __m128i res_8 = _mm_packus_epi16(round_result, round_result);
        *(uint32_t *)(&dst0[0]) = _mm_cvtsi128_si32(res_8);
      } else {
        _mm_store_si128((__m128i *)(&dst[0]), res_unsigned);
      }
      src_ptr += src_stride;
      dst += dst_stride;
      dst0 += dst_stride0;
    } while (--h);
  } else {
    assert(!(w % 8));
    int i = 0;
    do {
      int j = 0;
      do {
        const __m128i data =
            _mm_loadu_si128((__m128i *)&src_ptr[i * src_stride + j]);
        __m128i s[4];

        // Filter even-index pixels
        s[0] = data;
        s[1] = _mm_srli_si128(data, 2);
        s[2] = _mm_srli_si128(data, 4);
        s[3] = _mm_srli_si128(data, 6);
        const __m128i res_even = convolve_lo_x(s, coeffs);

        // Filter odd-index pixels
        s[0] = _mm_srli_si128(data, 1);
        s[1] = _mm_srli_si128(data, 3);
        s[2] = _mm_srli_si128(data, 5);
        s[3] = _mm_srli_si128(data, 7);
        const __m128i res_odd = convolve_lo_x(s, coeffs);

        // Rearrange pixels back into the order 0 ... 7
        const __m128i res_lo = _mm_unpacklo_epi32(res_even, res_odd);
        const __m128i res_hi = _mm_unpackhi_epi32(res_even, res_odd);
        const __m128i res_lo_round =
            _mm_sra_epi32(_mm_add_epi32(res_lo, round_const), round_shift);
        const __m128i res_hi_round =
            _mm_sra_epi32(_mm_add_epi32(res_hi, round_const), round_shift);
        const __m128i res_lo_shift = _mm_sll_epi32(res_lo_round, left_shift);
        const __m128i res_hi_shift = _mm_sll_epi32(res_hi_round, left_shift);

        const __m128i res_16b = _mm_packs_epi32(res_lo_shift, res_hi_shift);
        const __m128i res_unsigned = _mm_add_epi16(res_16b, offset_const);

        // Accumulate values into the destination buffer
        if (do_average) {
          const __m128i data_ref_0 =
              _mm_loadu_si128((__m128i *)(&dst[i * dst_stride + j]));

          const __m128i comp_avg_res =
              comp_avg(&data_ref_0, &res_unsigned, &wt, use_jnt_comp_avg);

          const __m128i round_result = convolve_rounding(
              &comp_avg_res, &offset_const, &rounding_const, rounding_shift);

          const __m128i res_8 = _mm_packus_epi16(round_result, round_result);
          _mm_storel_epi64((__m128i *)(&dst0[i * dst_stride0 + j]), res_8);
        } else {
          _mm_store_si128((__m128i *)(&dst[i * dst_stride + j]), res_unsigned);
        }
        j += 8;
      } while (j < w);
    } while (++i < h);
  }
}

void av1_jnt_convolve_y_sse2(const uint8_t *src, int src_stride, uint8_t *dst0,
                             int dst_stride0, int w, int h,
                             const InterpFilterParams *filter_params_x,
                             const InterpFilterParams *filter_params_y,
                             const int subpel_x_q4, const int subpel_y_q4,
                             ConvolveParams *conv_params) {
  const int bd = 8;
  CONV_BUF_TYPE *dst = conv_params->dst;
  const int dst_stride = conv_params->dst_stride;
  const int fo_vert = filter_params_y->taps / 2 - 1;
  const uint8_t *src_ptr = src - fo_vert * src_stride;
  const int bits = FILTER_BITS - conv_params->round_0;
  const __m128i left_shift = _mm_cvtsi32_si128(bits);
  const __m128i wt0 = _mm_set1_epi16(conv_params->fwd_offset);
  const __m128i wt1 = _mm_set1_epi16(conv_params->bck_offset);
  const __m128i wt = _mm_unpacklo_epi16(wt0, wt1);
  const int do_average = conv_params->do_average;
  const int use_jnt_comp_avg = conv_params->use_jnt_comp_avg;
  const int offset_0 =
      bd + 2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  const int offset = (1 << offset_0) + (1 << (offset_0 - 1));
  const __m128i offset_const = _mm_set1_epi16(offset);
  const int rounding_shift =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  const __m128i rounding_const = _mm_set1_epi16((1 << rounding_shift) >> 1);
  const __m128i round_const = _mm_set1_epi32((1 << conv_params->round_1) >> 1);
  const __m128i round_shift = _mm_cvtsi32_si128(conv_params->round_1);
  __m128i coeffs[4];

  (void)filter_params_x;
  (void)subpel_x_q4;

  prepare_coeffs(filter_params_y, subpel_y_q4, coeffs);

  if (w == 4) {
    __m128i s[8], src6, res, res_shift;
    src6 = _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 6 * src_stride));
    s[0] = _mm_unpacklo_epi8(
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 0 * src_stride)),
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 1 * src_stride)));
    s[1] = _mm_unpacklo_epi8(
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 1 * src_stride)),
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 2 * src_stride)));
    s[2] = _mm_unpacklo_epi8(
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 2 * src_stride)),
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 3 * src_stride)));
    s[3] = _mm_unpacklo_epi8(
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 3 * src_stride)),
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 4 * src_stride)));
    s[4] = _mm_unpacklo_epi8(
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 4 * src_stride)),
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 5 * src_stride)));
    s[5] = _mm_unpacklo_epi8(
        _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 5 * src_stride)), src6);

    do {
      s[6] = _mm_unpacklo_epi8(
          src6, _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 7 * src_stride)));
      src6 = _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 8 * src_stride));
      s[7] = _mm_unpacklo_epi8(
          _mm_cvtsi32_si128(*(uint32_t *)(src_ptr + 7 * src_stride)), src6);

      res = convolve_lo_y(s + 0, coeffs);
      res_shift = _mm_sll_epi32(res, left_shift);
      res_shift =
          _mm_sra_epi32(_mm_add_epi32(res_shift, round_const), round_shift);

      __m128i res_16b = _mm_packs_epi32(res_shift, res_shift);
      __m128i res_unsigned = _mm_add_epi16(res_16b, offset_const);

      // Accumulate values into the destination buffer
      if (do_average) {
        const __m128i data_ref_0 = _mm_loadu_si128((__m128i *)dst);

        const __m128i comp_avg_res =
            comp_avg(&data_ref_0, &res_unsigned, &wt, use_jnt_comp_avg);

        const __m128i round_result = convolve_rounding(
            &comp_avg_res, &offset_const, &rounding_const, rounding_shift);

        const __m128i res_8 = _mm_packus_epi16(round_result, round_result);
        *(uint32_t *)(&dst0[0]) = _mm_cvtsi128_si32(res_8);

      } else {
        _mm_store_si128((__m128i *)dst, res_unsigned);
      }

      src_ptr += src_stride;
      dst += dst_stride;
      dst0 += dst_stride0;

      res = convolve_lo_y(s + 1, coeffs);
      res_shift = _mm_sll_epi32(res, left_shift);
      res_shift =
          _mm_sra_epi32(_mm_add_epi32(res_shift, round_const), round_shift);

      res_16b = _mm_packs_epi32(res_shift, res_shift);
      res_unsigned = _mm_add_epi16(res_16b, offset_const);

      // Accumulate values into the destination buffer
      if (do_average) {
        const __m128i data_ref_0 = _mm_loadu_si128((__m128i *)dst);

        const __m128i comp_avg_res =
            comp_avg(&data_ref_0, &res_unsigned, &wt, use_jnt_comp_avg);

        const __m128i round_result = convolve_rounding(
            &comp_avg_res, &offset_const, &rounding_const, rounding_shift);

        const __m128i res_8 = _mm_packus_epi16(round_result, round_result);
        *(uint32_t *)(&dst0[0]) = _mm_cvtsi128_si32(res_8);

      } else {
        _mm_store_si128((__m128i *)dst, res_unsigned);
      }

      src_ptr += src_stride;
      dst += dst_stride;
      dst0 += dst_stride0;

      s[0] = s[2];
      s[1] = s[3];
      s[2] = s[4];
      s[3] = s[5];
      s[4] = s[6];
      s[5] = s[7];
      h -= 2;
    } while (h);
  } else {
    assert(!(w % 8));
    int j = 0;
    do {
      __m128i s[8], src6, res_lo, res_hi, res_lo_shift, res_hi_shift;
      const uint8_t *data = &src_ptr[j];

      src6 = _mm_loadl_epi64((__m128i *)(data + 6 * src_stride));
      s[0] = _mm_unpacklo_epi8(
          _mm_loadl_epi64((__m128i *)(data + 0 * src_stride)),
          _mm_loadl_epi64((__m128i *)(data + 1 * src_stride)));
      s[1] = _mm_unpacklo_epi8(
          _mm_loadl_epi64((__m128i *)(data + 1 * src_stride)),
          _mm_loadl_epi64((__m128i *)(data + 2 * src_stride)));
      s[2] = _mm_unpacklo_epi8(
          _mm_loadl_epi64((__m128i *)(data + 2 * src_stride)),
          _mm_loadl_epi64((__m128i *)(data + 3 * src_stride)));
      s[3] = _mm_unpacklo_epi8(
          _mm_loadl_epi64((__m128i *)(data + 3 * src_stride)),
          _mm_loadl_epi64((__m128i *)(data + 4 * src_stride)));
      s[4] = _mm_unpacklo_epi8(
          _mm_loadl_epi64((__m128i *)(data + 4 * src_stride)),
          _mm_loadl_epi64((__m128i *)(data + 5 * src_stride)));
      s[5] = _mm_unpacklo_epi8(
          _mm_loadl_epi64((__m128i *)(data + 5 * src_stride)), src6);

      int i = 0;
      do {
        data = &src_ptr[i * src_stride + j];
        s[6] = _mm_unpacklo_epi8(
            src6, _mm_loadl_epi64((__m128i *)(data + 7 * src_stride)));
        src6 = _mm_loadl_epi64((__m128i *)(data + 8 * src_stride));
        s[7] = _mm_unpacklo_epi8(
            _mm_loadl_epi64((__m128i *)(data + 7 * src_stride)), src6);

        res_lo = convolve_lo_y(s, coeffs);  // Filter low index pixels
        res_hi = convolve_hi_y(s, coeffs);  // Filter high index pixels
        res_lo_shift = _mm_sll_epi32(res_lo, left_shift);
        res_hi_shift = _mm_sll_epi32(res_hi, left_shift);
        res_lo_shift = _mm_sra_epi32(_mm_add_epi32(res_lo_shift, round_const),
                                     round_shift);
        res_hi_shift = _mm_sra_epi32(_mm_add_epi32(res_hi_shift, round_const),
                                     round_shift);

        __m128i res_16b = _mm_packs_epi32(res_lo_shift, res_hi_shift);
        __m128i res_unsigned = _mm_add_epi16(res_16b, offset_const);

        // Accumulate values into the destination buffer
        if (do_average) {
          const __m128i data_ref_0 =
              _mm_loadu_si128((__m128i *)(&dst[i * dst_stride + j]));

          const __m128i comp_avg_res =
              comp_avg(&data_ref_0, &res_unsigned, &wt, use_jnt_comp_avg);

          const __m128i round_result = convolve_rounding(
              &comp_avg_res, &offset_const, &rounding_const, rounding_shift);

          const __m128i res_8 = _mm_packus_epi16(round_result, round_result);
          _mm_storel_epi64((__m128i *)(&dst0[i * dst_stride0 + j]), res_8);
        } else {
          _mm_store_si128((__m128i *)(&dst[i * dst_stride + j]), res_unsigned);
        }
        i++;

        res_lo = convolve_lo_y(s + 1, coeffs);  // Filter low index pixels
        res_hi = convolve_hi_y(s + 1, coeffs);  // Filter high index pixels
        res_lo_shift = _mm_sll_epi32(res_lo, left_shift);
        res_hi_shift = _mm_sll_epi32(res_hi, left_shift);
        res_lo_shift = _mm_sra_epi32(_mm_add_epi32(res_lo_shift, round_const),
                                     round_shift);
        res_hi_shift = _mm_sra_epi32(_mm_add_epi32(res_hi_shift, round_const),
                                     round_shift);
        res_16b = _mm_packs_epi32(res_lo_shift, res_hi_shift);
        res_unsigned = _mm_add_epi16(res_16b, offset_const);

        // Accumulate values into the destination buffer
        if (do_average) {
          __m128i data_ref_0 =
              _mm_loadu_si128((__m128i *)(&dst[i * dst_stride + j]));

          const __m128i comp_avg_res =
              comp_avg(&data_ref_0, &res_unsigned, &wt, use_jnt_comp_avg);

          const __m128i round_result = convolve_rounding(
              &comp_avg_res, &offset_const, &rounding_const, rounding_shift);

          const __m128i res_8 = _mm_packus_epi16(round_result, round_result);
          _mm_storel_epi64((__m128i *)(&dst0[i * dst_stride0 + j]), res_8);
        } else {
          _mm_store_si128((__m128i *)(&dst[i * dst_stride + j]), res_unsigned);
        }
        i++;

        s[0] = s[2];
        s[1] = s[3];
        s[2] = s[4];
        s[3] = s[5];
        s[4] = s[6];
        s[5] = s[7];
      } while (i < h);
      j += 8;
    } while (j < w);
  }
}

void av1_jnt_convolve_2d_sse2(const uint8_t *src, int src_stride, uint8_t *dst0,
                              int dst_stride0, int w, int h,
                              const InterpFilterParams *filter_params_x,
                              const InterpFilterParams *filter_params_y,
                              const int subpel_x_q4, const int subpel_y_q4,
                              ConvolveParams *conv_params) {
  CONV_BUF_TYPE *dst = conv_params->dst;
  int dst_stride = conv_params->dst_stride;
  const int bd = 8;

  DECLARE_ALIGNED(16, int16_t,
                  im_block[(MAX_SB_SIZE + MAX_FILTER_TAP - 1) * MAX_SB_SIZE]);
  int im_h = h + filter_params_y->taps - 1;
  int im_stride = MAX_SB_SIZE;
  int i, j;
  const int fo_vert = filter_params_y->taps / 2 - 1;
  const int fo_horiz = filter_params_x->taps / 2 - 1;
  const int do_average = conv_params->do_average;
  const int use_jnt_comp_avg = conv_params->use_jnt_comp_avg;
  const uint8_t *const src_ptr = src - fo_vert * src_stride - fo_horiz;

  const __m128i zero = _mm_setzero_si128();

  const int w0 = conv_params->fwd_offset;
  const int w1 = conv_params->bck_offset;
  const __m128i wt0 = _mm_set1_epi16(w0);
  const __m128i wt1 = _mm_set1_epi16(w1);
  const __m128i wt = _mm_unpacklo_epi16(wt0, wt1);

  const int offset_0 =
      bd + 2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  const int offset = (1 << offset_0) + (1 << (offset_0 - 1));
  const __m128i offset_const = _mm_set1_epi16(offset);
  const int rounding_shift =
      2 * FILTER_BITS - conv_params->round_0 - conv_params->round_1;
  const __m128i rounding_const = _mm_set1_epi16((1 << rounding_shift) >> 1);

  /* Horizontal filter */
  {
    const int16_t *x_filter = av1_get_interp_filter_subpel_kernel(
        filter_params_x, subpel_x_q4 & SUBPEL_MASK);
    const __m128i coeffs_x = _mm_loadu_si128((__m128i *)x_filter);

    // coeffs 0 1 0 1 2 3 2 3
    const __m128i tmp_0 = _mm_unpacklo_epi32(coeffs_x, coeffs_x);
    // coeffs 4 5 4 5 6 7 6 7
    const __m128i tmp_1 = _mm_unpackhi_epi32(coeffs_x, coeffs_x);

    // coeffs 0 1 0 1 0 1 0 1
    const __m128i coeff_01 = _mm_unpacklo_epi64(tmp_0, tmp_0);
    // coeffs 2 3 2 3 2 3 2 3
    const __m128i coeff_23 = _mm_unpackhi_epi64(tmp_0, tmp_0);
    // coeffs 4 5 4 5 4 5 4 5
    const __m128i coeff_45 = _mm_unpacklo_epi64(tmp_1, tmp_1);
    // coeffs 6 7 6 7 6 7 6 7
    const __m128i coeff_67 = _mm_unpackhi_epi64(tmp_1, tmp_1);

    const __m128i round_const = _mm_set1_epi32(
        ((1 << conv_params->round_0) >> 1) + (1 << (bd + FILTER_BITS - 1)));
    const __m128i round_shift = _mm_cvtsi32_si128(conv_params->round_0);

    for (i = 0; i < im_h; ++i) {
      for (j = 0; j < w; j += 8) {
        __m128i temp_lo, temp_hi;
        const __m128i data =
            _mm_loadu_si128((__m128i *)&src_ptr[i * src_stride + j]);

        const __m128i src_lo = _mm_unpacklo_epi8(data, zero);
        const __m128i src_hi = _mm_unpackhi_epi8(data, zero);

        // Filter even-index pixels
        const __m128i res_0 = _mm_madd_epi16(src_lo, coeff_01);
        temp_lo = _mm_srli_si128(src_lo, 4);
        temp_hi = _mm_slli_si128(src_hi, 12);
        const __m128i src_2 = _mm_or_si128(temp_hi, temp_lo);
        const __m128i res_2 = _mm_madd_epi16(src_2, coeff_23);
        temp_lo = _mm_srli_si128(src_lo, 8);
        temp_hi = _mm_slli_si128(src_hi, 8);
        const __m128i src_4 = _mm_or_si128(temp_hi, temp_lo);
        const __m128i res_4 = _mm_madd_epi16(src_4, coeff_45);
        temp_lo = _mm_srli_si128(src_lo, 12);
        temp_hi = _mm_slli_si128(src_hi, 4);
        const __m128i src_6 = _mm_or_si128(temp_hi, temp_lo);
        const __m128i res_6 = _mm_madd_epi16(src_6, coeff_67);

        __m128i res_even = _mm_add_epi32(_mm_add_epi32(res_0, res_4),
                                         _mm_add_epi32(res_2, res_6));
        res_even =
            _mm_sra_epi32(_mm_add_epi32(res_even, round_const), round_shift);

        // Filter odd-index pixels
        temp_lo = _mm_srli_si128(src_lo, 2);
        temp_hi = _mm_slli_si128(src_hi, 14);
        const __m128i src_1 = _mm_or_si128(temp_hi, temp_lo);
        const __m128i res_1 = _mm_madd_epi16(src_1, coeff_01);
        temp_lo = _mm_srli_si128(src_lo, 6);
        temp_hi = _mm_slli_si128(src_hi, 10);
        const __m128i src_3 = _mm_or_si128(temp_hi, temp_lo);
        const __m128i res_3 = _mm_madd_epi16(src_3, coeff_23);
        temp_lo = _mm_srli_si128(src_lo, 10);
        temp_hi = _mm_slli_si128(src_hi, 6);
        const __m128i src_5 = _mm_or_si128(temp_hi, temp_lo);
        const __m128i res_5 = _mm_madd_epi16(src_5, coeff_45);
        temp_lo = _mm_srli_si128(src_lo, 14);
        temp_hi = _mm_slli_si128(src_hi, 2);
        const __m128i src_7 = _mm_or_si128(temp_hi, temp_lo);
        const __m128i res_7 = _mm_madd_epi16(src_7, coeff_67);

        __m128i res_odd = _mm_add_epi32(_mm_add_epi32(res_1, res_5),
                                        _mm_add_epi32(res_3, res_7));
        res_odd =
            _mm_sra_epi32(_mm_add_epi32(res_odd, round_const), round_shift);

        // Pack in the column order 0, 2, 4, 6, 1, 3, 5, 7
        __m128i res = _mm_packs_epi32(res_even, res_odd);
        _mm_store_si128((__m128i *)&im_block[i * im_stride + j], res);
      }
    }
  }

  /* Vertical filter */
  {
    const int16_t *y_filter = av1_get_interp_filter_subpel_kernel(
        filter_params_y, subpel_y_q4 & SUBPEL_MASK);
    const __m128i coeffs_y = _mm_loadu_si128((__m128i *)y_filter);

    // coeffs 0 1 0 1 2 3 2 3
    const __m128i tmp_0 = _mm_unpacklo_epi32(coeffs_y, coeffs_y);
    // coeffs 4 5 4 5 6 7 6 7
    const __m128i tmp_1 = _mm_unpackhi_epi32(coeffs_y, coeffs_y);

    // coeffs 0 1 0 1 0 1 0 1
    const __m128i coeff_01 = _mm_unpacklo_epi64(tmp_0, tmp_0);
    // coeffs 2 3 2 3 2 3 2 3
    const __m128i coeff_23 = _mm_unpackhi_epi64(tmp_0, tmp_0);
    // coeffs 4 5 4 5 4 5 4 5
    const __m128i coeff_45 = _mm_unpacklo_epi64(tmp_1, tmp_1);
    // coeffs 6 7 6 7 6 7 6 7
    const __m128i coeff_67 = _mm_unpackhi_epi64(tmp_1, tmp_1);

    const __m128i round_const = _mm_set1_epi32(
        ((1 << conv_params->round_1) >> 1) -
        (1 << (bd + 2 * FILTER_BITS - conv_params->round_0 - 1)));
    const __m128i round_shift = _mm_cvtsi32_si128(conv_params->round_1);

    for (i = 0; i < h; ++i) {
      for (j = 0; j < w; j += 8) {
        // Filter even-index pixels
        const int16_t *data = &im_block[i * im_stride + j];
        const __m128i src_0 =
            _mm_unpacklo_epi16(*(__m128i *)(data + 0 * im_stride),
                               *(__m128i *)(data + 1 * im_stride));
        const __m128i src_2 =
            _mm_unpacklo_epi16(*(__m128i *)(data + 2 * im_stride),
                               *(__m128i *)(data + 3 * im_stride));
        const __m128i src_4 =
            _mm_unpacklo_epi16(*(__m128i *)(data + 4 * im_stride),
                               *(__m128i *)(data + 5 * im_stride));
        const __m128i src_6 =
            _mm_unpacklo_epi16(*(__m128i *)(data + 6 * im_stride),
                               *(__m128i *)(data + 7 * im_stride));

        const __m128i res_0 = _mm_madd_epi16(src_0, coeff_01);
        const __m128i res_2 = _mm_madd_epi16(src_2, coeff_23);
        const __m128i res_4 = _mm_madd_epi16(src_4, coeff_45);
        const __m128i res_6 = _mm_madd_epi16(src_6, coeff_67);

        const __m128i res_even = _mm_add_epi32(_mm_add_epi32(res_0, res_2),
                                               _mm_add_epi32(res_4, res_6));

        // Filter odd-index pixels
        const __m128i src_1 =
            _mm_unpackhi_epi16(*(__m128i *)(data + 0 * im_stride),
                               *(__m128i *)(data + 1 * im_stride));
        const __m128i src_3 =
            _mm_unpackhi_epi16(*(__m128i *)(data + 2 * im_stride),
                               *(__m128i *)(data + 3 * im_stride));
        const __m128i src_5 =
            _mm_unpackhi_epi16(*(__m128i *)(data + 4 * im_stride),
                               *(__m128i *)(data + 5 * im_stride));
        const __m128i src_7 =
            _mm_unpackhi_epi16(*(__m128i *)(data + 6 * im_stride),
                               *(__m128i *)(data + 7 * im_stride));

        const __m128i res_1 = _mm_madd_epi16(src_1, coeff_01);
        const __m128i res_3 = _mm_madd_epi16(src_3, coeff_23);
        const __m128i res_5 = _mm_madd_epi16(src_5, coeff_45);
        const __m128i res_7 = _mm_madd_epi16(src_7, coeff_67);

        const __m128i res_odd = _mm_add_epi32(_mm_add_epi32(res_1, res_3),
                                              _mm_add_epi32(res_5, res_7));

        // Rearrange pixels back into the order 0 ... 7
        const __m128i res_lo = _mm_unpacklo_epi32(res_even, res_odd);
        const __m128i res_hi = _mm_unpackhi_epi32(res_even, res_odd);

        const __m128i res_lo_round =
            _mm_sra_epi32(_mm_add_epi32(res_lo, round_const), round_shift);
        const __m128i res_hi_round =
            _mm_sra_epi32(_mm_add_epi32(res_hi, round_const), round_shift);

        const __m128i res_16b = _mm_packs_epi32(res_lo_round, res_hi_round);
        const __m128i res_unsigned = _mm_add_epi16(res_16b, offset_const);

        // Accumulate values into the destination buffer
        if (do_average) {
          const __m128i data_ref_0 =
              _mm_loadu_si128((__m128i *)(&dst[i * dst_stride + j]));

          const __m128i comp_avg_res =
              comp_avg(&data_ref_0, &res_unsigned, &wt, use_jnt_comp_avg);

          const __m128i round_result = convolve_rounding(
              &comp_avg_res, &offset_const, &rounding_const, rounding_shift);

          const __m128i res_8 = _mm_packus_epi16(round_result, round_result);

          if (w > 4)
            _mm_storel_epi64((__m128i *)(&dst0[i * dst_stride0 + j]), res_8);
          else
            *(uint32_t *)(&dst0[i * dst_stride0 + j]) =
                _mm_cvtsi128_si32(res_8);
        } else {
          _mm_store_si128((__m128i *)(&dst[i * dst_stride + j]), res_unsigned);
        }
      }
    }
  }
}
