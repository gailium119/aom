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

#include "av1/common/cfl.h"
#include "av1/common/common_data.h"
#include "av1/common/onyxc_int.h"

#include "./av1_rtcd.h"

void cfl_init(CFL_CTX *cfl, AV1_COMMON *cm) {
  assert(block_size_wide[CFL_MAX_BLOCK_SIZE] == CFL_BUF_LINE);
  assert(block_size_high[CFL_MAX_BLOCK_SIZE] == CFL_BUF_LINE);
  if (!(cm->subsampling_x == 0 && cm->subsampling_y == 0) &&
      !(cm->subsampling_x == 1 && cm->subsampling_y == 1) &&
      !(cm->subsampling_x == 1 && cm->subsampling_y == 0)) {
    aom_internal_error(&cm->error, AOM_CODEC_UNSUP_BITSTREAM,
                       "Only 4:4:4, 4:2:2 and 4:2:0 are currently supported by "
                       "CfL, %d %d subsampling is not supported.\n",
                       cm->subsampling_x, cm->subsampling_y);
  }
  memset(&cfl->pred_buf_q3, 0, sizeof(cfl->pred_buf_q3));
  cfl->subsampling_x = cm->subsampling_x;
  cfl->subsampling_y = cm->subsampling_y;
  cfl->are_parameters_computed = 0;
  cfl->store_y = 0;
  // The DC_PRED cache is disabled by default and is only enabled in
  // cfl_rd_pick_alpha
  cfl->use_dc_pred_cache = 0;
  cfl->dc_pred_is_cached[CFL_PRED_U] = 0;
  cfl->dc_pred_is_cached[CFL_PRED_V] = 0;
}

void cfl_store_dc_pred(MACROBLOCKD *const xd, const uint8_t *input,
                       CFL_PRED_TYPE pred_plane, int width) {
  assert(pred_plane < CFL_PRED_PLANES);
  assert(width <= CFL_BUF_LINE);

  if (get_bitdepth_data_path_index(xd)) {
    uint16_t *const input_16 = CONVERT_TO_SHORTPTR(input);
    memcpy(xd->cfl.dc_pred_cache[pred_plane], input_16, width << 1);
    return;
  }

  memcpy(xd->cfl.dc_pred_cache[pred_plane], input, width);
}

static void cfl_load_dc_pred_lbd(const int16_t *dc_pred_cache, uint8_t *dst,
                                 int dst_stride, int width, int height) {
  for (int j = 0; j < height; j++) {
    memcpy(dst, dc_pred_cache, width);
    dst += dst_stride;
  }
}

static void cfl_load_dc_pred_hbd(const int16_t *dc_pred_cache, uint16_t *dst,
                                 int dst_stride, int width, int height) {
  const size_t num_bytes = width << 1;
  for (int j = 0; j < height; j++) {
    memcpy(dst, dc_pred_cache, num_bytes);
    dst += dst_stride;
  }
}
void cfl_load_dc_pred(MACROBLOCKD *const xd, uint8_t *dst, int dst_stride,
                      TX_SIZE tx_size, CFL_PRED_TYPE pred_plane) {
  const int width = tx_size_wide[tx_size];
  const int height = tx_size_high[tx_size];
  assert(pred_plane < CFL_PRED_PLANES);
  assert(width <= CFL_BUF_LINE);
  assert(height <= CFL_BUF_LINE);
  if (get_bitdepth_data_path_index(xd)) {
    uint16_t *dst_16 = CONVERT_TO_SHORTPTR(dst);
    cfl_load_dc_pred_hbd(xd->cfl.dc_pred_cache[pred_plane], dst_16, dst_stride,
                         width, height);
    return;
  }
  cfl_load_dc_pred_lbd(xd->cfl.dc_pred_cache[pred_plane], dst, dst_stride,
                       width, height);
}

// Due to frame boundary issues, it is possible that the total area covered by
// chroma exceeds that of luma. When this happens, we fill the missing pixels by
// repeating the last columns and/or rows.
static INLINE void cfl_pad(CFL_CTX *cfl, int width, int height) {
  const int diff_width = width - cfl->buf_width;
  const int diff_height = height - cfl->buf_height;

  if (diff_width > 0) {
    const int min_height = height - diff_height;
    int16_t *pred_buf_q3 = cfl->pred_buf_q3 + (width - diff_width);
    for (int j = 0; j < min_height; j++) {
      const int16_t last_pixel = pred_buf_q3[-1];
      assert(pred_buf_q3 + diff_width <= cfl->pred_buf_q3 + CFL_BUF_SQUARE);
      for (int i = 0; i < diff_width; i++) {
        pred_buf_q3[i] = last_pixel;
      }
      pred_buf_q3 += CFL_BUF_LINE;
    }
    cfl->buf_width = width;
  }
  if (diff_height > 0) {
    int16_t *pred_buf_q3 =
        cfl->pred_buf_q3 + ((height - diff_height) * CFL_BUF_LINE);
    for (int j = 0; j < diff_height; j++) {
      const int16_t *last_row_q3 = pred_buf_q3 - CFL_BUF_LINE;
      assert(pred_buf_q3 + width <= cfl->pred_buf_q3 + CFL_BUF_SQUARE);
      for (int i = 0; i < width; i++) {
        pred_buf_q3[i] = last_row_q3[i];
      }
      pred_buf_q3 += CFL_BUF_LINE;
    }
    cfl->buf_height = height;
  }
}

static void subtract_average_c(int16_t *pred_buf_q3, int width, int height,
                               int round_offset, int num_pel_log2) {
  int sum_q3 = 0;
  int16_t *pred_buf = pred_buf_q3;
  for (int j = 0; j < height; j++) {
    // assert(pred_buf_q3 + tx_width <= cfl->pred_buf_q3 + CFL_BUF_SQUARE);
    for (int i = 0; i < width; i++) {
      sum_q3 += pred_buf[i];
    }
    pred_buf += CFL_BUF_LINE;
  }
  const int avg_q3 = (sum_q3 + round_offset) >> num_pel_log2;
  // Loss is never more than 1/2 (in Q3)
  // assert(abs((avg_q3 * (1 << num_pel_log2)) - sum_q3) <= 1 << num_pel_log2 >>
  //       1);
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      pred_buf_q3[i] -= avg_q3;
    }
    pred_buf_q3 += CFL_BUF_LINE;
  }
}

static void cfl_subtract_averages_lossless(CFL_CTX *cfl, TX_SIZE tx_size) {
  const int width = cfl->buf_width;
  const int height = cfl->buf_height;
  const int tx_height = tx_size_high[tx_size];
  const int tx_width = tx_size_wide[tx_size];
  const int block_row_stride = CFL_BUF_LINE << tx_size_high_log2[tx_size];
  int16_t *pred_buf_q3 = cfl->pred_buf_q3;
  for (int b_j = 0; b_j < height; b_j += tx_height) {
    for (int b_i = 0; b_i < width; b_i += tx_width) {
      get_subtract_average_fn(tx_size)(pred_buf_q3 + b_i);
    }
    pred_buf_q3 += block_row_stride;
  }
}

CFL_SUB_AVG_FN(c)

static INLINE int cfl_idx_to_alpha(int alpha_idx, int joint_sign,
                                   CFL_PRED_TYPE pred_type) {
  const int alpha_sign = (pred_type == CFL_PRED_U) ? CFL_SIGN_U(joint_sign)
                                                   : CFL_SIGN_V(joint_sign);
  if (alpha_sign == CFL_SIGN_ZERO) return 0;
  const int abs_alpha_q3 =
      (pred_type == CFL_PRED_U) ? CFL_IDX_U(alpha_idx) : CFL_IDX_V(alpha_idx);
  return (alpha_sign == CFL_SIGN_POS) ? abs_alpha_q3 + 1 : -abs_alpha_q3 - 1;
}

static INLINE void cfl_predict_lbd_c(const int16_t *pred_buf_q3, uint8_t *dst,
                                     int dst_stride, int alpha_q3, int width,
                                     int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      dst[i] =
          clip_pixel(get_scaled_luma_q0(alpha_q3, pred_buf_q3[i]) + dst[i]);
    }
    dst += dst_stride;
    pred_buf_q3 += CFL_BUF_LINE;
  }
}

// Null function used for invalid tx_sizes
void cfl_predict_lbd_null(const int16_t *pred_buf_q3, uint8_t *dst,
                          int dst_stride, int alpha_q3) {
  (void)pred_buf_q3;
  (void)dst;
  (void)dst_stride;
  (void)alpha_q3;
  assert(0);
}

CFL_PREDICT_FN(c, lbd)

void cfl_predict_hbd_c(const int16_t *pred_buf_q3, uint16_t *dst,
                       int dst_stride, int alpha_q3, int bit_depth, int width,
                       int height) {
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      dst[i] = clip_pixel_highbd(
          get_scaled_luma_q0(alpha_q3, pred_buf_q3[i]) + dst[i], bit_depth);
    }
    dst += dst_stride;
    pred_buf_q3 += CFL_BUF_LINE;
  }
}

// Null function used for invalid tx_sizes
void cfl_predict_hbd_null(const int16_t *pred_buf_q3, uint16_t *dst,
                          int dst_stride, int alpha_q3, int bd) {
  (void)pred_buf_q3;
  (void)dst;
  (void)dst_stride;
  (void)alpha_q3;
  (void)bd;
  assert(0);
}

CFL_PREDICT_FN(c, hbd)

static void cfl_compute_parameters(MACROBLOCKD *const xd, TX_SIZE tx_size) {
  CFL_CTX *const cfl = &xd->cfl;
  const MB_MODE_INFO *const mbmi = &xd->mi[0]->mbmi;

  // Do not call cfl_compute_parameters multiple time on the same values.
  assert(cfl->are_parameters_computed == 0);

  if (xd->lossless[mbmi->segment_id]) {
    cfl_pad(cfl, block_size_wide[mbmi->sb_type],
            block_size_high[mbmi->sb_type]);
    cfl_subtract_averages_lossless(cfl, tx_size);
  } else {
    cfl_pad(cfl, tx_size_wide[tx_size], tx_size_high[tx_size]);
    get_subtract_average_fn(tx_size)(cfl->pred_buf_q3);
  }
  cfl->are_parameters_computed = 1;
}

void cfl_predict_block(MACROBLOCKD *const xd, uint8_t *dst, int dst_stride,
                       TX_SIZE tx_size, int plane) {
  CFL_CTX *const cfl = &xd->cfl;
  MB_MODE_INFO *mbmi = &xd->mi[0]->mbmi;
  assert(is_cfl_allowed(mbmi));

  if (!cfl->are_parameters_computed) cfl_compute_parameters(xd, tx_size);

  const int alpha_q3 =
      cfl_idx_to_alpha(mbmi->cfl_alpha_idx, mbmi->cfl_alpha_signs, plane - 1);
  assert((tx_size_high[tx_size] - 1) * CFL_BUF_LINE + tx_size_wide[tx_size] <=
         CFL_BUF_SQUARE);
  if (get_bitdepth_data_path_index(xd)) {
    uint16_t *dst_16 = CONVERT_TO_SHORTPTR(dst);
    get_predict_hbd_fn(tx_size)(cfl->pred_buf_q3, dst_16, dst_stride, alpha_q3,
                                xd->bd);
    return;
  }
  get_predict_lbd_fn(tx_size)(cfl->pred_buf_q3, dst, dst_stride, alpha_q3);
}

void cfl_predict_block_lossless(MACROBLOCKD *const xd, uint8_t *dst,
                                int dst_stride, int row, int col,
                                TX_SIZE tx_size, int plane) {
  const MB_MODE_INFO *const mbmi = &xd->mi[0]->mbmi;
  CFL_CTX *const cfl = &xd->cfl;
  assert(is_cfl_allowed(mbmi));

  if (!cfl->are_parameters_computed) cfl_compute_parameters(xd, tx_size);

  const int alpha_q3 =
      cfl_idx_to_alpha(mbmi->cfl_alpha_idx, mbmi->cfl_alpha_signs, plane - 1);
  assert(((row << tx_size_high_log2[0]) + tx_size_high[tx_size] - 1) *
                 CFL_BUF_LINE +
             (col << tx_size_wide_log2[0]) + tx_size_wide[tx_size] <=
         CFL_BUF_SQUARE);
  const int16_t *pred_buf_q3 =
      cfl->pred_buf_q3 + ((row * CFL_BUF_LINE + col) << tx_size_wide_log2[0]);
  if (get_bitdepth_data_path_index(xd)) {
    uint16_t *dst_16 = CONVERT_TO_SHORTPTR(dst);
    get_predict_hbd_fn(tx_size)(pred_buf_q3, dst_16, dst_stride, alpha_q3,
                                xd->bd);
    return;
  }
  get_predict_lbd_fn(tx_size)(pred_buf_q3, dst, dst_stride, alpha_q3);
}

// Null function used for invalid tx_sizes
void cfl_subsample_lbd_null(const uint8_t *input, int input_stride,
                            int16_t *output_q3) {
  (void)input;
  (void)input_stride;
  (void)output_q3;
  assert(0);
}

// Null function used for invalid tx_sizes
void cfl_subsample_hbd_null(const uint16_t *input, int input_stride,
                            int16_t *output_q3) {
  (void)input;
  (void)input_stride;
  (void)output_q3;
  assert(0);
}

static void cfl_luma_subsampling_420_lbd_c(const uint8_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j += 2) {
    for (int i = 0; i < width; i += 2) {
      const int bot = i + input_stride;
      output_q3[i >> 1] =
          (input[i] + input[i + 1] + input[bot] + input[bot + 1]) << 1;
    }
    input += input_stride << 1;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_422_lbd_c(const uint8_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  assert((height - 1) * CFL_BUF_LINE + width <= CFL_BUF_SQUARE);
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i += 2) {
      output_q3[i >> 1] = (input[i] + input[i + 1]) << 2;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_444_lbd_c(const uint8_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  assert((height - 1) * CFL_BUF_LINE + width <= CFL_BUF_SQUARE);
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      output_q3[i] = input[i] << 3;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_420_hbd_c(const uint16_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  for (int j = 0; j < height; j += 2) {
    for (int i = 0; i < width; i += 2) {
      const int bot = i + input_stride;
      output_q3[i >> 1] =
          (input[i] + input[i + 1] + input[bot] + input[bot + 1]) << 1;
    }
    input += input_stride << 1;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_422_hbd_c(const uint16_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  assert((height - 1) * CFL_BUF_LINE + width <= CFL_BUF_SQUARE);
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i += 2) {
      output_q3[i >> 1] = (input[i] + input[i + 1]) << 2;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

static void cfl_luma_subsampling_444_hbd_c(const uint16_t *input,
                                           int input_stride, int16_t *output_q3,
                                           int width, int height) {
  assert((height - 1) * CFL_BUF_LINE + width <= CFL_BUF_SQUARE);
  for (int j = 0; j < height; j++) {
    for (int i = 0; i < width; i++) {
      output_q3[i] = input[i] << 3;
    }
    input += input_stride;
    output_q3 += CFL_BUF_LINE;
  }
}

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 422 SIMD
// will be implemented
CFL_SUBSAMPLE_FUNCTIONS(c, 422, lbd)

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
CFL_SUBSAMPLE_FUNCTIONS(c, 444, lbd)

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
CFL_SUBSAMPLE_FUNCTIONS(c, 420, hbd)

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
CFL_SUBSAMPLE_FUNCTIONS(c, 422, hbd)

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
CFL_SUBSAMPLE_FUNCTIONS(c, 444, hbd)

CFL_GET_SUBSAMPLE_FUNCTION(c)

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
cfl_subsample_hbd_fn cfl_get_luma_subsampling_420_hbd_c(TX_SIZE tx_size) {
  CFL_SUBSAMPLE_FUNCTION_ARRAY(c, 420, hbd)
  return subfn_420[tx_size];
}

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
cfl_subsample_hbd_fn cfl_get_luma_subsampling_422_hbd_c(TX_SIZE tx_size) {
  CFL_SUBSAMPLE_FUNCTION_ARRAY(c, 422, hbd)
  return subfn_422[tx_size];
}

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
cfl_subsample_hbd_fn cfl_get_luma_subsampling_444_hbd_c(TX_SIZE tx_size) {
  CFL_SUBSAMPLE_FUNCTION_ARRAY(c, 444, hbd)
  return subfn_444[tx_size];
}

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 422 SIMD
// will be implemented
cfl_subsample_lbd_fn cfl_get_luma_subsampling_422_lbd_c(TX_SIZE tx_size) {
  CFL_SUBSAMPLE_FUNCTION_ARRAY(c, 422, lbd)
  return subfn_422[tx_size];
}

// TODO(ltrudeau) Move into the CFL_GET_SUBSAMPLE_FUNCTION when LBD 444 SIMD
// will be implemented
cfl_subsample_lbd_fn cfl_get_luma_subsampling_444_lbd_c(TX_SIZE tx_size) {
  CFL_SUBSAMPLE_FUNCTION_ARRAY(c, 444, lbd)
  return subfn_444[tx_size];
}

static INLINE cfl_subsample_hbd_fn cfl_subsampling_hbd(TX_SIZE tx_size,
                                                       int sub_x, int sub_y) {
  if (sub_x == 1) {
    if (sub_y == 1) {
      // TODO(ltrudeau) Remove _c when HBD 420 SIMD is added
      return cfl_get_luma_subsampling_420_hbd_c(tx_size);
    }
    // TODO(ltrudeau) Remove _c when HBD 422 SIMD is added
    return cfl_get_luma_subsampling_422_hbd_c(tx_size);
  }
  // TODO(ltrudeau) Remove _c when HBD 444 SIMD is added
  return cfl_get_luma_subsampling_444_hbd_c(tx_size);
}

static INLINE cfl_subsample_lbd_fn cfl_subsampling_lbd(TX_SIZE tx_size,
                                                       int sub_x, int sub_y) {
  if (sub_x == 1) {
    if (sub_y == 1) {
      return cfl_get_luma_subsampling_420_lbd(tx_size);
    }
    // TODO(ltrudeau) Remove _c when LBD 422 SIMD is added
    return cfl_get_luma_subsampling_422_lbd_c(tx_size);
  }
  // TODO(ltrudeau) Remove _c when LBD 444 SIMD is added
  return cfl_get_luma_subsampling_444_lbd_c(tx_size);
}

static void cfl_store(CFL_CTX *cfl, const uint8_t *input, int input_stride,
                      int row, int col, TX_SIZE tx_size, int use_hbd) {
  const int width = tx_size_wide[tx_size];
  const int height = tx_size_high[tx_size];
  const int tx_off_log2 = tx_size_wide_log2[0];
  const int sub_x = cfl->subsampling_x;
  const int sub_y = cfl->subsampling_y;
  const int store_row = row << (tx_off_log2 - sub_y);
  const int store_col = col << (tx_off_log2 - sub_x);
  const int store_height = height >> sub_y;
  const int store_width = width >> sub_x;

  // Invalidate current parameters
  cfl->are_parameters_computed = 0;

  // Store the surface of the pixel buffer that was written to, this way we
  // can manage chroma overrun (e.g. when the chroma surfaces goes beyond the
  // frame boundary)
  if (col == 0 && row == 0) {
    cfl->buf_width = store_width;
    cfl->buf_height = store_height;
  } else {
    cfl->buf_width = OD_MAXI(store_col + store_width, cfl->buf_width);
    cfl->buf_height = OD_MAXI(store_row + store_height, cfl->buf_height);
  }

  // Check that we will remain inside the pixel buffer.
  assert(store_row + store_height <= CFL_BUF_LINE);
  assert(store_col + store_width <= CFL_BUF_LINE);

  // Store the input into the CfL pixel buffer
  int16_t *pred_buf_q3 =
      cfl->pred_buf_q3 + (store_row * CFL_BUF_LINE + store_col);

  if (use_hbd) {
    cfl_subsampling_hbd(tx_size, sub_x, sub_y)(CONVERT_TO_SHORTPTR(input),
                                               input_stride, pred_buf_q3);
  } else {
    cfl_subsampling_lbd(tx_size, sub_x, sub_y)(input, input_stride,
                                               pred_buf_q3);
  }
}

// Adjust the row and column of blocks smaller than 8X8, as chroma-referenced
// and non-chroma-referenced blocks are stored together in the CfL buffer.
static INLINE void sub8x8_adjust_offset(const CFL_CTX *cfl, int *row_out,
                                        int *col_out) {
  // Increment row index for bottom: 8x4, 16x4 or both bottom 4x4s.
  if ((cfl->mi_row & 0x01) && cfl->subsampling_y) {
    assert(*row_out == 0);
    (*row_out)++;
  }

  // Increment col index for right: 4x8, 4x16 or both right 4x4s.
  if ((cfl->mi_col & 0x01) && cfl->subsampling_x) {
    assert(*col_out == 0);
    (*col_out)++;
  }
}

void cfl_store_tx(MACROBLOCKD *const xd, int row, int col, TX_SIZE tx_size,
                  BLOCK_SIZE bsize) {
  CFL_CTX *const cfl = &xd->cfl;
  struct macroblockd_plane *const pd = &xd->plane[AOM_PLANE_Y];
  uint8_t *dst =
      &pd->dst.buf[(row * pd->dst.stride + col) << tx_size_wide_log2[0]];

  assert(is_cfl_allowed(&xd->mi[0]->mbmi));
  if (block_size_high[bsize] == 4 || block_size_wide[bsize] == 4) {
    // Only dimensions of size 4 can have an odd offset.
    assert(!((col & 1) && tx_size_wide[tx_size] != 4));
    assert(!((row & 1) && tx_size_high[tx_size] != 4));
    sub8x8_adjust_offset(cfl, &row, &col);
  }
  cfl_store(cfl, dst, pd->dst.stride, row, col, tx_size,
            get_bitdepth_data_path_index(xd));
}

void cfl_store_block(MACROBLOCKD *const xd, BLOCK_SIZE bsize, TX_SIZE tx_size) {
  CFL_CTX *const cfl = &xd->cfl;
  struct macroblockd_plane *const pd = &xd->plane[AOM_PLANE_Y];
  int row = 0;
  int col = 0;

  assert(is_cfl_allowed(&xd->mi[0]->mbmi));
  if (block_size_high[bsize] == 4 || block_size_wide[bsize] == 4) {
    sub8x8_adjust_offset(cfl, &row, &col);
  }
  const int width = max_intra_block_width(xd, bsize, AOM_PLANE_Y, tx_size);
  const int height = max_intra_block_height(xd, bsize, AOM_PLANE_Y, tx_size);
  tx_size = get_tx_size(width, height);
  cfl_store(cfl, pd->dst.buf, pd->dst.stride, row, col, tx_size,
            get_bitdepth_data_path_index(xd));
}
