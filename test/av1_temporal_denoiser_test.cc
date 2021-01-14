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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <tuple>

#include "config/av1_rtcd.h"

#include "test/acm_random.h"
#include "test/clear_system_state.h"
#include "test/util.h"
#include "test/register_state_check.h"

#include "aom_scale/yv12config.h"
#include "aom/aom_integer.h"
#include "av1/common/reconinter.h"
#include "av1/encoder/context_tree.h"
#include "av1/encoder/av1_temporal_denoiser.h"

using libaom_test::ACMRandom;

namespace {

const int kNumPixels = 128 * 128;

typedef int (*Av1DenoiserFilterFunc)(const uint8_t *sig, int sig_stride,
                                     const uint8_t *mc_avg, int mc_avg_stride,
                                     uint8_t *avg, int avg_stride,
                                     int increase_denoising, BLOCK_SIZE bs,
                                     int motion_magnitude);
typedef std::tuple<Av1DenoiserFilterFunc, BLOCK_SIZE> AV1DenoiserTestParam;

class AV1DenoiserTest
    : public ::testing::Test,
      public ::testing::WithParamInterface<AV1DenoiserTestParam> {
 public:
  virtual ~AV1DenoiserTest() {}

  virtual void SetUp() { bs_ = GET_PARAM(1); }

  virtual void TearDown() { libaom_test::ClearSystemState(); }

 protected:
  BLOCK_SIZE bs_;
};

TEST_P(AV1DenoiserTest, BitexactCheck) {
  ACMRandom rnd(ACMRandom::DeterministicSeed());
  const int count_test_block = 4000;

  // Allocate the space for input and output,
  // where sig_block is the block to be denoised,
  // mc_avg_block is the denoised reference block,
  // avg_block_c is the denoised result from C code,
  // avg_block_sse2 is the denoised result from SSE2 code.
  DECLARE_ALIGNED(16, uint8_t, sig_block[kNumPixels]);
  DECLARE_ALIGNED(16, uint8_t, mc_avg_block[kNumPixels]);
  DECLARE_ALIGNED(16, uint8_t, avg_block_c[kNumPixels]);
  DECLARE_ALIGNED(16, uint8_t, avg_block_sse2[kNumPixels]);

  for (int i = 0; i < count_test_block; ++i) {
    // Generate random motion magnitude, 20% of which exceed the threshold.
    const int motion_magnitude_random =
        rnd.Rand8() % static_cast<int>(MOTION_MAGNITUDE_THRESHOLD * 1.2);

    // Initialize a test block with random number in range [0, 255].
    for (int j = 0; j < kNumPixels; ++j) {
      int temp = 0;
      sig_block[j] = rnd.Rand8();
      // The pixels in mc_avg_block are generated by adding a random
      // number in range [-19, 19] to corresponding pixels in sig_block.
      temp =
          sig_block[j] + ((rnd.Rand8() % 2 == 0) ? -1 : 1) * (rnd.Rand8() % 20);
      // Clip.
      mc_avg_block[j] = (temp < 0) ? 0 : ((temp > 255) ? 255 : temp);
    }

    ASM_REGISTER_STATE_CHECK(
        av1_denoiser_filter_c(sig_block, 128, mc_avg_block, 128, avg_block_c,
                              128, 0, bs_, motion_magnitude_random));

    ASM_REGISTER_STATE_CHECK(GET_PARAM(0)(sig_block, 128, mc_avg_block, 128,
                                          avg_block_sse2, 128, 0, bs_,
                                          motion_magnitude_random));

    // Test bitexactness.
    for (int h = 0; h < block_size_high[bs_]; ++h) {
      for (int w = 0; w < block_size_wide[bs_]; ++w) {
        EXPECT_EQ(avg_block_c[h * 128 + w], avg_block_sse2[h * 128 + w]);
      }
    }
  }
}

using std::make_tuple;

// Test for all block size.
#if HAVE_SSE2
INSTANTIATE_TEST_SUITE_P(
    SSE2, AV1DenoiserTest,
    ::testing::Values(make_tuple(&av1_denoiser_filter_sse2, BLOCK_8X8),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_8X16),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_16X8),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_16X16),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_16X32),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_32X16),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_32X32),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_32X64),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_64X32),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_64X64),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_128X64),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_64X128),
                      make_tuple(&av1_denoiser_filter_sse2, BLOCK_128X128)));
#endif  // HAVE_SSE2

#if HAVE_NEON
INSTANTIATE_TEST_SUITE_P(
    NEON, AV1DenoiserTest,
    ::testing::Values(make_tuple(&av1_denoiser_filter_neon, BLOCK_8X8),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_8X16),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_16X8),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_16X16),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_16X32),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_32X16),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_32X32),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_32X64),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_64X32),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_64X64),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_128X64),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_64X128),
                      make_tuple(&av1_denoiser_filter_neon, BLOCK_128X128)));
#endif
}  // namespace
