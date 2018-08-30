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

#ifndef RAWENC_H_
#define RAWENC_H_

#include "aom/aom_decoder.h"
#include "common/md5_utils.h"
#include "common/tools_common.h"

#ifdef __cplusplus
extern "C" {
#endif

static void raw_update_image_md5(const aom_image_t *img, const int planes[3],
                                 MD5Context *md5);
static void raw_write_image_file(const aom_image_t *img, const int *planes,
                                 const int num_planes, FILE *file);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // RAWENC_H_
