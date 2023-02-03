/*
 * Copyright (c) 2022, Alliance for Open Media. All rights reserved
 *
 * This source code is subject to the terms of the BSD 2 Clause License and
 * the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
 * was not distributed with this source code in the LICENSE file, you can
 * obtain it at www.aomedia.org/license/software. If the Alliance for Open
 * Media Patent License 1.0 was not distributed with this source code in the
 * PATENTS file, you can obtain it at www.aomedia.org/license/patent.
 */

#ifndef AOM_AV1_ENCODER_SALIENCY_MAP_H_
#define AOM_AV1_ENCODER_SALIENCY_MAP_H_

// The Gabor filter is generated by setting the parameters as:
// ksize = 9
// sigma = 1
// theta = y*np.pi/4, where y /in {0, 1, 2, 3}
// lambda1 = 1
// gamma=0.8
// phi =0
static const double GaborFilter0[9][9] = {

  { 2.0047323e-06, 6.6387620e-05, 8.0876675e-04, 3.6246411e-03, 5.9760227e-03,
    3.6246411e-03, 8.0876675e-04, 6.6387620e-05, 2.0047323e-06 },
  { 1.8831115e-05, 6.2360091e-04, 7.5970138e-03, 3.4047455e-02, 5.6134764e-02,
    3.4047455e-02, 7.5970138e-03, 6.2360091e-04, 1.8831115e-05 },
  { 9.3271126e-05, 3.0887155e-03, 3.7628256e-02, 1.6863814e-01, 2.7803731e-01,
    1.6863814e-01, 3.7628256e-02, 3.0887155e-03, 9.3271126e-05 },
  { 2.4359586e-04, 8.0667874e-03, 9.8273583e-02, 4.4043165e-01, 7.2614902e-01,
    4.4043165e-01, 9.8273583e-02, 8.0667874e-03, 2.4359586e-04 },
  { 3.3546262e-04, 1.1108996e-02, 1.3533528e-01, 6.0653067e-01, 1.0000000e+00,
    6.0653067e-01, 1.3533528e-01, 1.1108996e-02, 3.3546262e-04 },
  { 2.4359586e-04, 8.0667874e-03, 9.8273583e-02, 4.4043165e-01, 7.2614902e-01,
    4.4043165e-01, 9.8273583e-02, 8.0667874e-03, 2.4359586e-04 },
  { 9.3271126e-05, 3.0887155e-03, 3.7628256e-02, 1.6863814e-01, 2.7803731e-01,
    1.6863814e-01, 3.7628256e-02, 3.0887155e-03, 9.3271126e-05 },
  { 1.8831115e-05, 6.2360091e-04, 7.5970138e-03, 3.4047455e-02, 5.6134764e-02,
    3.4047455e-02, 7.5970138e-03, 6.2360091e-04, 1.8831115e-05 },
  { 2.0047323e-06, 6.6387620e-05, 8.0876675e-04, 3.6246411e-03, 5.9760227e-03,
    3.6246411e-03, 8.0876675e-04, 6.6387620e-05, 2.0047323e-06 }

};

static const double GaborFilter45[9][9] = {

  { -6.2165498e-08, 3.8760313e-06, 3.0079011e-06, -4.4602581e-04, 6.6981313e-04,
    1.3962291e-03, -9.9486928e-04, -8.1631159e-05, 3.5712848e-05 },
  { 3.8760313e-06, 5.7044272e-06, -1.6041942e-03, 4.5687673e-03, 1.8061366e-02,
    -2.4406660e-02, -3.7979286e-03, 3.1511115e-03, -8.1631159e-05 },
  { 3.0079011e-06, -1.6041942e-03, 8.6645801e-03, 6.4960226e-02, -1.6647682e-01,
    -4.9129307e-02, 7.7304743e-02, -3.7979286e-03, -9.9486928e-04 },
  { -4.4602581e-04, 4.5687673e-03, 6.4960226e-02, -3.1572008e-01,
    -1.7670043e-01, 5.2729243e-01, -4.9129307e-02, -2.4406660e-02,
    1.3962291e-03 },
  { 6.6981313e-04, 1.8061366e-02, -1.6647682e-01, -1.7670043e-01, 1.0000000e+00,
    -1.7670043e-01, -1.6647682e-01, 1.8061366e-02, 6.6981313e-04 },
  { 1.3962291e-03, -2.4406660e-02, -4.9129307e-02, 5.2729243e-01,
    -1.7670043e-01, -3.1572008e-01, 6.4960226e-02, 4.5687673e-03,
    -4.4602581e-04 },
  { -9.9486928e-04, -3.7979286e-03, 7.7304743e-02, -4.9129307e-02,
    -1.6647682e-01, 6.4960226e-02, 8.6645801e-03, -1.6041942e-03,
    3.0079011e-06 },
  { -8.1631159e-05, 3.1511115e-03, -3.7979286e-03, -2.4406660e-02,
    1.8061366e-02, 4.5687673e-03, -1.6041942e-03, 5.7044272e-06,
    3.8760313e-06 },
  { 3.5712848e-05, -8.1631159e-05, -9.9486928e-04, 1.3962291e-03, 6.6981313e-04,
    -4.4602581e-04, 3.0079011e-06, 3.8760313e-06, -6.2165498e-08 }

};

static const double GaborFilter90[9][9] = {

  { 2.0047323e-06, 1.8831115e-05, 9.3271126e-05, 2.4359586e-04, 3.3546262e-04,
    2.4359586e-04, 9.3271126e-05, 1.8831115e-05, 2.0047323e-06 },
  { 6.6387620e-05, 6.2360091e-04, 3.0887155e-03, 8.0667874e-03, 1.1108996e-02,
    8.0667874e-03, 3.0887155e-03, 6.2360091e-04, 6.6387620e-05 },
  { 8.0876675e-04, 7.5970138e-03, 3.7628256e-02, 9.8273583e-02, 1.3533528e-01,
    9.8273583e-02, 3.7628256e-02, 7.5970138e-03, 8.0876675e-04 },
  { 3.6246411e-03, 3.4047455e-02, 1.6863814e-01, 4.4043165e-01, 6.0653067e-01,
    4.4043165e-01, 1.6863814e-01, 3.4047455e-02, 3.6246411e-03 },
  { 5.9760227e-03, 5.6134764e-02, 2.7803731e-01, 7.2614902e-01, 1.0000000e+00,
    7.2614902e-01, 2.7803731e-01, 5.6134764e-02, 5.9760227e-03 },
  { 3.6246411e-03, 3.4047455e-02, 1.6863814e-01, 4.4043165e-01, 6.0653067e-01,
    4.4043165e-01, 1.6863814e-01, 3.4047455e-02, 3.6246411e-03 },
  { 8.0876675e-04, 7.5970138e-03, 3.7628256e-02, 9.8273583e-02, 1.3533528e-01,
    9.8273583e-02, 3.7628256e-02, 7.5970138e-03, 8.0876675e-04 },
  { 6.6387620e-05, 6.2360091e-04, 3.0887155e-03, 8.0667874e-03, 1.1108996e-02,
    8.0667874e-03, 3.0887155e-03, 6.2360091e-04, 6.6387620e-05 },
  { 2.0047323e-06, 1.8831115e-05, 9.3271126e-05, 2.4359586e-04, 3.3546262e-04,
    2.4359586e-04, 9.3271126e-05, 1.8831115e-05, 2.0047323e-06 }

};

static const double GaborFilter135[9][9] = {

  { 3.5712848e-05, -8.1631159e-05, -9.9486928e-04, 1.3962291e-03, 6.6981313e-04,
    -4.4602581e-04, 3.0079011e-06, 3.8760313e-06, -6.2165498e-08 },
  { -8.1631159e-05, 3.1511115e-03, -3.7979286e-03, -2.4406660e-02,
    1.8061366e-02, 4.5687673e-03, -1.6041942e-03, 5.7044272e-06,
    3.8760313e-06 },
  { -9.9486928e-04, -3.7979286e-03, 7.7304743e-02, -4.9129307e-02,
    -1.6647682e-01, 6.4960226e-02, 8.6645801e-03, -1.6041942e-03,
    3.0079011e-06 },
  { 1.3962291e-03, -2.4406660e-02, -4.9129307e-02, 5.2729243e-01,
    -1.7670043e-01, -3.1572008e-01, 6.4960226e-02, 4.5687673e-03,
    -4.4602581e-04 },
  { 6.6981313e-04, 1.8061366e-02, -1.6647682e-01, -1.7670043e-01, 1.0000000e+00,
    -1.7670043e-01, -1.6647682e-01, 1.8061366e-02, 6.6981313e-04 },
  { -4.4602581e-04, 4.5687673e-03, 6.4960226e-02, -3.1572008e-01,
    -1.7670043e-01, 5.2729243e-01, -4.9129307e-02, -2.4406660e-02,
    1.3962291e-03 },
  { 3.0079011e-06, -1.6041942e-03, 8.6645801e-03, 6.4960226e-02, -1.6647682e-01,
    -4.9129307e-02, 7.7304743e-02, -3.7979286e-03, -9.9486928e-04 },
  { 3.8760313e-06, 5.7044272e-06, -1.6041942e-03, 4.5687673e-03, 1.8061366e-02,
    -2.4406660e-02, -3.7979286e-03, 3.1511115e-03, -8.1631159e-05 },
  { -6.2165498e-08, 3.8760313e-06, 3.0079011e-06, -4.4602581e-04, 6.6981313e-04,
    1.3962291e-03, -9.9486928e-04, -8.1631159e-05, 3.5712848e-05 }

};

typedef struct saliency_feature_map {
  double *buf;
  int height;
  int width;
} saliency_feature_map;

void set_saliency_map(AV1_COMP *cpi);

#endif  // AOM_AV1_ENCODER_TUNE_VMAF_H_