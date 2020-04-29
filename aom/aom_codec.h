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

///////////////////////////////////////////////////////////////////////////////
// Internal implementation details
///////////////////////////////////////////////////////////////////////////////
//
// There are two levels of interfaces used to access the AOM codec: the
// the aom_codec_iface and the aom_codec_ctx.
//
// 1. aom_codec_iface_t
//    (Related files: aom/aom_codec.h, aom/src/aom_codec.c,
//    aom/internal/aom_codec_internal.h, av1/av1_cx_iface.c,
//    av1/av1_dx_iface.c)
//
// Used to initialize the codec context, which contains the configuration for
// for modifying the encoder/decoder during run-time. See the other
// documentation in this header file for more details. For the most part,
// users will call helper functions, such as aom_codec_iface_name,
// aom_codec_get_caps, etc., to interact with it.
//
// The main purpose of the aom_codec_iface_t is to provide a way to generate
// a default codec config, find out what capabilities the implementation has,
// and create an aom_codec_ctx_t (which is actually used to interact with the
// codec).
//
// Note that the implementations for the AV1 algorithm are located in
// av1/av1_cx_iface.c and av1/av1_dx_iface.c
//
//
// 2. aom_codec_ctx_t
//  (Related files: aom/aom_codec.h, av1/av1_cx_iface.c, av1/av1_dx_iface.c,
//   aom/aomcx.h, aom/aomdx.h, aom/src/aom_encoder.c, aom/src/aom_decoder.c)
//
// The actual interface between user code and the codec. It stores the name
// of the codec, a pointer back to the aom_codec_iface_t that initialized it,
// initialization flags, a config for either encoder or the decoder, and a
// pointer to internal data.
//
// The codec is configured / queried through calls to aom_codec_control,
// which takes a control code (listed in aomcx.h and aomdx.h) and a parameter.
// In the case of "getter" control codes, the parameter is modified to have
// the requested value; in the case of "setter" control codes, the codec's
// configuration is changed based on the parameter. Note that a aom_codec_err_t
// is returned, which indicates if the operation was successful or not.
//
// Note that for the encoder, the aom_codec_alg_priv_t points to the
// the aom_codec_alg_priv structure in av1/av1_cx_iface.c, and for the decoder,
// the struct in av1/av1_dx_iface.c. Variables such as AV1_COMP cpi are stored
// here and also used in the core algorithm.
//
// At the end, aom_codec_destroy should be called for each initialized
// aom_codec_ctx_t.

/*!\defgroup codec Common Algorithm Interface
 * This abstraction allows applications to easily support multiple video
 * formats with minimal code duplication. This section describes the interface
 * common to all codecs (both encoders and decoders).
 * @{
 */

/*!\file
 * \brief Describes the codec algorithm interface to applications.
 *
 * This file describes the interface between an application and a
 * video codec algorithm.
 *
 * An application instantiates a specific codec instance by using
 * aom_codec_dec_init() or aom_codec_enc_init() and a pointer to the
 * algorithm's interface structure:
 *     <pre>
 *     my_app.c:
 *       extern aom_codec_iface_t my_codec;
 *       {
 *           aom_codec_ctx_t algo;
 *           int threads = 4;
 *           aom_codec_dec_cfg_t cfg = { threads, 0, 0, 1 };
 *           res = aom_codec_dec_init(&algo, &my_codec, &cfg, 0);
 *       }
 *     </pre>
 *
 * Once initialized, the instance is managed using other functions from
 * the aom_codec_* family.
 */
#ifndef AOM_AOM_AOM_CODEC_H_
#define AOM_AOM_AOM_CODEC_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "aom/aom_image.h"
#include "aom/aom_integer.h"

/*!\brief Decorator indicating a function is deprecated */
#ifndef AOM_DEPRECATED
#if defined(__GNUC__) && __GNUC__
#define AOM_DEPRECATED __attribute__((deprecated))
#elif defined(_MSC_VER)
#define AOM_DEPRECATED
#else
#define AOM_DEPRECATED
#endif
#endif /* AOM_DEPRECATED */

#ifndef AOM_DECLSPEC_DEPRECATED
#if defined(__GNUC__) && __GNUC__
#define AOM_DECLSPEC_DEPRECATED /**< \copydoc #AOM_DEPRECATED */
#elif defined(_MSC_VER)
/*!\brief \copydoc #AOM_DEPRECATED */
#define AOM_DECLSPEC_DEPRECATED __declspec(deprecated)
#else
#define AOM_DECLSPEC_DEPRECATED /**< \copydoc #AOM_DEPRECATED */
#endif
#endif /* AOM_DECLSPEC_DEPRECATED */

/*!\brief Decorator indicating a function is potentially unused */
#ifdef AOM_UNUSED
#elif defined(__GNUC__) || defined(__clang__)
#define AOM_UNUSED __attribute__((unused))
#else
#define AOM_UNUSED
#endif

/*!\brief Decorator indicating that given struct/union/enum is packed */
#ifndef ATTRIBUTE_PACKED
#if defined(__GNUC__) && __GNUC__
#define ATTRIBUTE_PACKED __attribute__((packed))
#elif defined(_MSC_VER)
#define ATTRIBUTE_PACKED
#else
#define ATTRIBUTE_PACKED
#endif
#endif /* ATTRIBUTE_PACKED */

/*!\brief Current ABI version number
 *
 * \internal
 * If this file is altered in any way that changes the ABI, this value
 * must be bumped.  Examples include, but are not limited to, changing
 * types, removing or reassigning enums, adding/removing/rearranging
 * fields to structures
 */
#define AOM_CODEC_ABI_VERSION (5 + AOM_IMAGE_ABI_VERSION) /**<\hideinitializer*/

/*!\brief Algorithm return codes */
typedef enum {
  /*!\brief Operation completed without error */
  AOM_CODEC_OK,

  /*!\brief Unspecified error */
  AOM_CODEC_ERROR,

  /*!\brief Memory operation failed */
  AOM_CODEC_MEM_ERROR,

  /*!\brief ABI version mismatch */
  AOM_CODEC_ABI_MISMATCH,

  /*!\brief Algorithm does not have required capability */
  AOM_CODEC_INCAPABLE,

  /*!\brief The given bitstream is not supported.
   *
   * The bitstream was unable to be parsed at the highest level. The decoder
   * is unable to proceed. This error \ref SHOULD be treated as fatal to the
   * stream. */
  AOM_CODEC_UNSUP_BITSTREAM,

  /*!\brief Encoded bitstream uses an unsupported feature
   *
   * The decoder does not implement a feature required by the encoder. This
   * return code should only be used for features that prevent future
   * pictures from being properly decoded. This error \ref MAY be treated as
   * fatal to the stream or \ref MAY be treated as fatal to the current GOP.
   */
  AOM_CODEC_UNSUP_FEATURE,

  /*!\brief The coded data for this stream is corrupt or incomplete
   *
   * There was a problem decoding the current frame.  This return code
   * should only be used for failures that prevent future pictures from
   * being properly decoded. This error \ref MAY be treated as fatal to the
   * stream or \ref MAY be treated as fatal to the current GOP. If decoding
   * is continued for the current GOP, artifacts may be present.
   */
  AOM_CODEC_CORRUPT_FRAME,

  /*!\brief An application-supplied parameter is invalid.
   *
   */
  AOM_CODEC_INVALID_PARAM,

  /*!\brief An iterator reached the end of list.
   *
   */
  AOM_CODEC_LIST_END

} aom_codec_err_t;

/*! \brief Codec capabilities bitfield
 *
 *  Each codec advertises the capabilities it supports as part of its
 *  ::aom_codec_iface_t interface structure. Capabilities are extra interfaces
 *  or functionality, and are not required to be supported.
 *
 *  The available flags are specified by AOM_CODEC_CAP_* defines.
 */
typedef long aom_codec_caps_t;
#define AOM_CODEC_CAP_DECODER 0x1 /**< Is a decoder */
#define AOM_CODEC_CAP_ENCODER 0x2 /**< Is an encoder */

/*! \brief Initialization-time Feature Enabling
 *
 *  Certain codec features must be known at initialization time, to allow for
 *  proper memory allocation.
 *
 *  The available flags are specified by AOM_CODEC_USE_* defines.
 */
typedef long aom_codec_flags_t;

/*!\brief Time Stamp Type
 *
 * An integer, which when multiplied by the stream's time base, provides
 * the absolute time of a sample.
 */
typedef int64_t aom_codec_pts_t;

/*!\brief Codec interface structure.
 *
 * Contains function pointers and other data private to the codec
 * implementation. This structure is opaque to the application. Common
 * functions used with this structure:
 *   - aom_codec_iface_name(aom_codec_iface_t *iface): get the
 *     name of the codec
 *   - aom_codec_get_caps(aom_codec_iface_t *iface): returns
 *     the capabilities of the codec
 *   - aom_codec_enc_config_default: generate the default config for
 *     initializing the encoder (see documention in aom_encoder.h)
 *   - aom_codec_dec_init, aom_codec_enc_init: initialize the codec context
 *     structure (see documentation on aom_codec_ctx).
 *
 * To get access to the AV1 encoder and decoder, use aom_codec_av1_cx() and
 *  aom_codec_av1_dx().
 */
typedef const struct aom_codec_iface aom_codec_iface_t;

/*!\brief Codec private data structure.
 *
 * Contains data private to the codec implementation. This structure is opaque
 * to the application.
 */
typedef struct aom_codec_priv aom_codec_priv_t;

/*!\brief Iterator
 *
 * Opaque storage used for iterating over lists.
 */
typedef const void *aom_codec_iter_t;

/*!\brief Codec context structure
 *
 * All codecs \ref MUST support this context structure fully. In general,
 * this data should be considered private to the codec algorithm, and
 * not be manipulated or examined by the calling application. Applications
 * may reference the 'name' member to get a printable description of the
 * algorithm.
 */
typedef struct aom_codec_ctx {
  const char *name;             /**< Printable interface name */
  aom_codec_iface_t *iface;     /**< Interface pointers */
  aom_codec_err_t err;          /**< Last returned error */
  const char *err_detail;       /**< Detailed info, if available */
  aom_codec_flags_t init_flags; /**< Flags passed at init time */
  union {
    /**< Decoder Configuration Pointer */
    const struct aom_codec_dec_cfg *dec;
    /**< Encoder Configuration Pointer */
    const struct aom_codec_enc_cfg *enc;
    const void *raw;
  } config;               /**< Configuration pointer aliasing union */
  aom_codec_priv_t *priv; /**< Algorithm private storage */
} aom_codec_ctx_t;

/*!\brief Bit depth for codec
 * *
 * This enumeration determines the bit depth of the codec.
 */
typedef enum aom_bit_depth {
  AOM_BITS_8 = 8,   /**<  8 bits */
  AOM_BITS_10 = 10, /**< 10 bits */
  AOM_BITS_12 = 12, /**< 12 bits */
} aom_bit_depth_t;

/*!\brief Superblock size selection.
 *
 * Defines the superblock size used for encoding. The superblock size can
 * either be fixed at 64x64 or 128x128 pixels, or it can be dynamically
 * selected by the encoder for each frame.
 */
typedef enum aom_superblock_size {
  AOM_SUPERBLOCK_SIZE_64X64,   /**< Always use 64x64 superblocks. */
  AOM_SUPERBLOCK_SIZE_128X128, /**< Always use 128x128 superblocks. */
  AOM_SUPERBLOCK_SIZE_DYNAMIC  /**< Select superblock size dynamically. */
} aom_superblock_size_t;

/*
 * Library Version Number Interface
 *
 * For example, see the following sample return values:
 *     aom_codec_version()           (1<<16 | 2<<8 | 3)
 *     aom_codec_version_str()       "v1.2.3-rc1-16-gec6a1ba"
 *     aom_codec_version_extra_str() "rc1-16-gec6a1ba"
 */

/*!\brief Return the version information (as an integer)
 *
 * Returns a packed encoding of the library version number. This will only
 * include the major.minor.patch component of the version number. Note that this
 * encoded value should be accessed through the macros provided, as the encoding
 * may change in the future.
 *
 */
int aom_codec_version(void);

/*!\brief Return the major version number */
#define aom_codec_version_major() ((aom_codec_version() >> 16) & 0xff)

/*!\brief Return the minor version number */
#define aom_codec_version_minor() ((aom_codec_version() >> 8) & 0xff)

/*!\brief Return the patch version number */
#define aom_codec_version_patch() ((aom_codec_version() >> 0) & 0xff)

/*!\brief Return the version information (as a string)
 *
 * Returns a printable string containing the full library version number. This
 * may contain additional text following the three digit version number, as to
 * indicate release candidates, prerelease versions, etc.
 *
 */
const char *aom_codec_version_str(void);

/*!\brief Return the version information (as a string)
 *
 * Returns a printable "extra string". This is the component of the string
 * returned by aom_codec_version_str() following the three digit version number.
 *
 */
const char *aom_codec_version_extra_str(void);

/*!\brief Return the build configuration
 *
 * Returns a printable string containing an encoded version of the build
 * configuration. This may be useful to aom support.
 *
 */
const char *aom_codec_build_config(void);

/*!\brief Return the name for a given interface
 *
 * Returns a human readable string for name of the given codec interface.
 *
 * \param[in]    iface     Interface pointer
 *
 */
const char *aom_codec_iface_name(aom_codec_iface_t *iface);

/*!\brief Convert error number to printable string
 *
 * Returns a human readable string for the last error returned by the
 * algorithm. The returned error will be one line and will not contain
 * any newline characters.
 *
 *
 * \param[in]    err     Error number.
 *
 */
const char *aom_codec_err_to_string(aom_codec_err_t err);

/*!\brief Retrieve error synopsis for codec context
 *
 * Returns a human readable string for the last error returned by the
 * algorithm. The returned error will be one line and will not contain
 * any newline characters.
 *
 *
 * \param[in]    ctx     Pointer to this instance's context.
 *
 */
const char *aom_codec_error(aom_codec_ctx_t *ctx);

/*!\brief Retrieve detailed error information for codec context
 *
 * Returns a human readable string providing detailed information about
 * the last error.
 *
 * \param[in]    ctx     Pointer to this instance's context.
 *
 * \retval NULL
 *     No detailed information is available.
 */
const char *aom_codec_error_detail(aom_codec_ctx_t *ctx);

/* REQUIRED FUNCTIONS
 *
 * The following functions are required to be implemented for all codecs.
 * They represent the base case functionality expected of all codecs.
 */

/*!\brief Destroy a codec instance
 *
 * Destroys a codec context, freeing any associated memory buffers.
 *
 * \param[in] ctx   Pointer to this instance's context
 *
 * \retval #AOM_CODEC_OK
 *     The codec algorithm initialized.
 * \retval #AOM_CODEC_MEM_ERROR
 *     Memory allocation failed.
 */
aom_codec_err_t aom_codec_destroy(aom_codec_ctx_t *ctx);

/*!\brief Get the capabilities of an algorithm.
 *
 * Retrieves the capabilities bitfield from the algorithm's interface.
 *
 * \param[in] iface   Pointer to the algorithm interface
 *
 */
aom_codec_caps_t aom_codec_get_caps(aom_codec_iface_t *iface);

/*!\name Control Algorithms
 *
 * Theses function are used to exchange algorithm specific data with the codec
 * instance. These can be used to implement features specific to a particular
 * algorithm.
 *
 * If additional sanity checks are desired, there are functions
 * aom_codec_control_set_X and aom_codec_control_get_X,
 * where X can be "int", "uint", or "ptr". These type-checking versions verify
 * that the control code has either SET or GET in its name (as appropriate),
 * and that the control code matches the expected data type; it then calls
 * aom_codec_control. Note that the "ptr" version also takes the size of the
 * underlying object, as an additional sanity check. Also note that the
 * "get_ptr" version should write directly into the pointed memory. Also note
 * that these versions also assert() that the right control code is passed in
 * (if assertions are turned off, they will return AOM_CODEC_INVALID_PARAM).
 *
 * All of these functions return an aom_codec_err_t; AOM_CODEC_OK means the
 * result was processed, AOM_CODEC_ERROR means it was not processed, and
 * AOM_CODEC_INVALID_PARAM means that it was not processed because the data was
 * invalid. If ctx is non-NULL, then ctx->err will be set.
 *
 * The codec control identifiers can be found in aom.h, aomcx.h, and aomdx.h
 * (defined as aom_com_control_id, aome_enc_control_id, and aom_dec_control_id).
 * @{
 */
/*!\brief Generic Control Algorithm
 *
 * aom_codec_control takes a context, a control code, and a variable
 * number of parameters (which should always be a single parameter -- var-args
 * are used to work-around type-checking).
 *
 * \param[in]     ctx              Pointer to this instance's context
 * \param[in]     ctrl_id          Algorithm specific control identifier
 *
 * \retval #AOM_CODEC_OK
 *     The control request was processed.
 * \retval #AOM_CODEC_ERROR
 *     The control request was not processed.
 * \retval #AOM_CODEC_INVALID_PARAM
 *     The data was not valid.
 */
aom_codec_err_t aom_codec_control(aom_codec_ctx_t *ctx, int ctrl_id, ...);
/*!\brief Type-checked setter version for an int parameter. */
aom_codec_err_t aom_codec_control_set_int(aom_codec_ctx_t *ctx, int ctrl_id,
                                          int val);
/*!\brief Type-checked getter version for an int parameter. */
aom_codec_err_t aom_codec_control_get_int(aom_codec_ctx_t *ctx, int ctrl_id,
                                          int *val);
/*!\brief Type-checked setter version for an unsigned int parameter. */
aom_codec_err_t aom_codec_control_set_uint(aom_codec_ctx_t *ctx, int ctrl_id,
                                           unsigned int val);
/*!\brief Type-checked getter version for an unsigned int parameter. */
aom_codec_err_t aom_codec_control_get_uint(aom_codec_ctx_t *ctx, int ctrl_id,
                                           unsigned int *val);
/*!\brief Sanity checked setter version for a pointer to a data structure. */
aom_codec_err_t aom_codec_control_set_ptr(aom_codec_ctx_t *ctx, int ctrl_id,
                                          const void *ptr, size_t obj_size);
/*!\brief Sanity checked getter version for a pointer to a data structure. */
aom_codec_err_t aom_codec_control_get_ptr(aom_codec_ctx_t *ctx, int ctrl_id,
                                          void *ptr, size_t obj_size);
/*!@} - end control algorithm member group */

/*!\brief OBU types. */
typedef enum ATTRIBUTE_PACKED {
  OBU_SEQUENCE_HEADER = 1,
  OBU_TEMPORAL_DELIMITER = 2,
  OBU_FRAME_HEADER = 3,
  OBU_TILE_GROUP = 4,
  OBU_METADATA = 5,
  OBU_FRAME = 6,
  OBU_REDUNDANT_FRAME_HEADER = 7,
  OBU_TILE_LIST = 8,
  OBU_PADDING = 15,
} OBU_TYPE;

/*!\brief OBU metadata types. */
typedef enum {
  OBU_METADATA_TYPE_AOM_RESERVED_0 = 0,
  OBU_METADATA_TYPE_HDR_CLL = 1,
  OBU_METADATA_TYPE_HDR_MDCV = 2,
  OBU_METADATA_TYPE_SCALABILITY = 3,
  OBU_METADATA_TYPE_ITUT_T35 = 4,
  OBU_METADATA_TYPE_TIMECODE = 5,
} OBU_METADATA_TYPE;

/*!\brief Returns string representation of OBU_TYPE.
 *
 * \param[in]     type            The OBU_TYPE to convert to string.
 */
const char *aom_obu_type_to_string(OBU_TYPE type);

/*!@} - end defgroup codec*/
#ifdef __cplusplus
}
#endif
#endif  // AOM_AOM_AOM_CODEC_H_
