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

/*!\defgroup vp8_decoder WebM VP8/VP9 Decoder
 * \ingroup vp8
 *
 * @{
 */
/*!\file
 * \brief Provides definitions for using VP8 or VP9 within the aom Decoder
 *        interface.
 */
#ifndef VPX_VP8DX_H_
#define VPX_VP8DX_H_

#ifdef __cplusplus
extern "C" {
#endif

/* Include controls common to both the encoder and decoder */
#include "./vp8.h"

/*!\name Algorithm interface for AV1
 *
 * This interface provides the capability to decode AV1 streams.
 * @{
 */
extern aom_codec_iface_t aom_codec_av1_dx_algo;
extern aom_codec_iface_t *aom_codec_av1_dx(void);
/*!@} - end algorithm interface member group*/

/*!\enum vp8_dec_control_id
 * \brief VP8 decoder control functions
 *
 * This set of macros define the control functions available for the VP8
 * decoder interface.
 *
 * \sa #aom_codec_control
 */
enum vp8_dec_control_id {
  /** control function to get info on which reference frames were updated
   *  by the last decode
   */
  VP8D_GET_LAST_REF_UPDATES = VP8_DECODER_CTRL_ID_START,

  /** check if the indicated frame is corrupted */
  VP8D_GET_FRAME_CORRUPTED,

  /** control function to get info on which reference frames were used
   *  by the last decode
   */
  VP8D_GET_LAST_REF_USED,

  /** decryption function to decrypt encoded buffer data immediately
   * before decoding. Takes a aom_decrypt_init, which contains
   * a callback function and opaque context pointer.
   */
  VPXD_SET_DECRYPTOR,
  VP8D_SET_DECRYPTOR = VPXD_SET_DECRYPTOR,

  /** control function to get the dimensions that the current frame is decoded
   * at. This may be different to the intended display size for the frame as
   * specified in the wrapper or frame header (see VP9D_GET_DISPLAY_SIZE). */
  VP9D_GET_FRAME_SIZE,

  /** control function to get the current frame's intended display dimensions
   * (as specified in the wrapper or frame header). This may be different to
   * the decoded dimensions of this frame (see VP9D_GET_FRAME_SIZE). */
  VP9D_GET_DISPLAY_SIZE,

  /** control function to get the bit depth of the stream. */
  VP9D_GET_BIT_DEPTH,

  /** control function to set the byte alignment of the planes in the reference
   * buffers. Valid values are power of 2, from 32 to 1024. A value of 0 sets
   * legacy alignment. I.e. Y plane is aligned to 32 bytes, U plane directly
   * follows Y plane, and V plane directly follows U plane. Default value is 0.
   */
  VP9_SET_BYTE_ALIGNMENT,

  /** control function to invert the decoding order to from right to left. The
   * function is used in a test to confirm the decoding independence of tile
   * columns. The function may be used in application where this order
   * of decoding is desired.
   *
   * TODO(yaowu): Rework the unit test that uses this control, and in a future
   *              release, this test-only control shall be removed.
   */
  VP9_INVERT_TILE_DECODE_ORDER,

  /** control function to set the skip loop filter flag. Valid values are
   * integers. The decoder will skip the loop filter when its value is set to
   * nonzero. If the loop filter is skipped the decoder may accumulate decode
   * artifacts. The default value is 0.
   */
  VP9_SET_SKIP_LOOP_FILTER,

  VP8_DECODER_CTRL_ID_MAX
};

/** Decrypt n bytes of data from input -> output, using the decrypt_state
 *  passed in VPXD_SET_DECRYPTOR.
 */
typedef void (*aom_decrypt_cb)(void *decrypt_state, const unsigned char *input,
                               unsigned char *output, int count);

/*!\brief Structure to hold decryption state
 *
 * Defines a structure to hold the decryption state and access function.
 */
typedef struct aom_decrypt_init {
  /*! Decrypt callback. */
  aom_decrypt_cb decrypt_cb;

  /*! Decryption state. */
  void *decrypt_state;
} aom_decrypt_init;

/*!\brief A deprecated alias for aom_decrypt_init.
 */
typedef aom_decrypt_init vp8_decrypt_init;

/*!\cond */
/*!\brief VP8 decoder control function parameter type
 *
 * Defines the data types that VP8D control functions take. Note that
 * additional common controls are defined in vp8.h
 *
 */

VPX_CTRL_USE_TYPE(VP8D_GET_LAST_REF_UPDATES, int *)
#define VPX_CTRL_VP8D_GET_LAST_REF_UPDATES
VPX_CTRL_USE_TYPE(VP8D_GET_FRAME_CORRUPTED, int *)
#define VPX_CTRL_VP8D_GET_FRAME_CORRUPTED
VPX_CTRL_USE_TYPE(VP8D_GET_LAST_REF_USED, int *)
#define VPX_CTRL_VP8D_GET_LAST_REF_USED
VPX_CTRL_USE_TYPE(VPXD_SET_DECRYPTOR, aom_decrypt_init *)
#define VPX_CTRL_VPXD_SET_DECRYPTOR
VPX_CTRL_USE_TYPE(VP8D_SET_DECRYPTOR, aom_decrypt_init *)
#define VPX_CTRL_VP8D_SET_DECRYPTOR
VPX_CTRL_USE_TYPE(VP9D_GET_DISPLAY_SIZE, int *)
#define VPX_CTRL_VP9D_GET_DISPLAY_SIZE
VPX_CTRL_USE_TYPE(VP9D_GET_BIT_DEPTH, unsigned int *)
#define VPX_CTRL_VP9D_GET_BIT_DEPTH
VPX_CTRL_USE_TYPE(VP9D_GET_FRAME_SIZE, int *)
#define VPX_CTRL_VP9D_GET_FRAME_SIZE
VPX_CTRL_USE_TYPE(VP9_INVERT_TILE_DECODE_ORDER, int)
#define VPX_CTRL_VP9_INVERT_TILE_DECODE_ORDER

/*!\endcond */
/*! @} - end defgroup vp8_decoder */

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // VPX_VP8DX_H_
