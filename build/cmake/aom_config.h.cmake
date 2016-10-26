/* Copyright (c) 2016, Alliance for Open Media. All rights reserved */
/*  */
/* This source code is subject to the terms of the BSD 2 Clause License and */
/* the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License */
/* was not distributed with this source code in the LICENSE file, you can */
/* obtain it at www.aomedia.org/license/software. If the Alliance for Open */
/* Media Patent License 1.0 was not distributed with this source code in the */
/* PATENTS file, you can obtain it at www.aomedia.org/license/patent. */

/* This file is processed by cmake and used to produce aom_config.h in the
   directory where cmake was executed. */
#ifndef AOM_CONFIG_H
#define AOM_CONFIG_H
#define RESTRICT ${RESTRICT}
#define INLINE ${INLINE}
#define ARCH_ARM ${ARCH_ARM}
#define ARCH_MIPS ${ARCH_MIPS}
#define ARCH_X86 ${ARCH_X86}
#define ARCH_X86_64 ${ARCH_X86_64}
#define HAVE_EDSP ${HAVE_EDSP}
#define HAVE_MEDIA ${HAVE_MEDIA}
#define HAVE_NEON ${HAVE_NEON}
#define HAVE_NEON_ASM ${HAVE_NEON_ASM}
#define HAVE_MIPS32 ${HAVE_MIPS32}
#define HAVE_DSPR2 ${HAVE_DSPR2}
#define HAVE_MSA ${HAVE_MSA}
#define HAVE_MIPS64 ${HAVE_MIPS64}
#define HAVE_MMX ${HAVE_MMX}
#define HAVE_SSE ${HAVE_SSE}
#define HAVE_SSE2 ${HAVE_SSE2}
#define HAVE_SSE3 ${HAVE_SSE3}
#define HAVE_SSSE3 ${HAVE_SSSE3}
#define HAVE_SSE4_1 ${HAVE_SSE4_1}
#define HAVE_AVX ${HAVE_AVX}
#define HAVE_AVX2 ${HAVE_AVX2}
#define HAVE_AOM_PORTS ${HAVE_AOM_PORTS}
#define HAVE_PTHREAD_H ${HAVE_PTHREAD_H}
#define HAVE_UNISTD_H ${HAVE_UNISTD_H}
#define CONFIG_DEPENDENCY_TRACKING ${CONFIG_DEPENDENCY_TRACKING}
#define CONFIG_EXTERNAL_BUILD ${CONFIG_EXTERNAL_BUILD}
#define CONFIG_INSTALL_DOCS ${CONFIG_INSTALL_DOCS}
#define CONFIG_INSTALL_BINS ${CONFIG_INSTALL_BINS}
#define CONFIG_INSTALL_LIBS ${CONFIG_INSTALL_LIBS}
#define CONFIG_INSTALL_SRCS ${CONFIG_INSTALL_SRCS}
#define CONFIG_USE_X86INC ${CONFIG_USE_X86INC}
#define CONFIG_DEBUG ${CONFIG_DEBUG}
#define CONFIG_GPROF ${CONFIG_GPROF}
#define CONFIG_GCOV ${CONFIG_GCOV}
#define CONFIG_RVCT ${CONFIG_RVCT}
#define CONFIG_GCC ${CONFIG_GCC}
#define CONFIG_MSVS ${CONFIG_MSVS}
#define CONFIG_PIC ${CONFIG_PIC}
#define CONFIG_BIG_ENDIAN ${CONFIG_BIG_ENDIAN}
#define CONFIG_CODEC_SRCS ${CONFIG_CODEC_SRCS}
#define CONFIG_DEBUG_LIBS ${CONFIG_DEBUG_LIBS}
#define CONFIG_DEQUANT_TOKENS ${CONFIG_DEQUANT_TOKENS}
#define CONFIG_DC_RECON ${CONFIG_DC_RECON}
#define CONFIG_RUNTIME_CPU_DETECT ${CONFIG_RUNTIME_CPU_DETECT}
#define CONFIG_MULTITHREAD ${CONFIG_MULTITHREAD}
#define CONFIG_INTERNAL_STATS ${CONFIG_INTERNAL_STATS}
#define CONFIG_AV1_ENCODER ${CONFIG_AV1_ENCODER}
#define CONFIG_AV1_DECODER ${CONFIG_AV1_DECODER}
#define CONFIG_AV1 ${CONFIG_AV1}
#define CONFIG_ENCODERS ${CONFIG_ENCODERS}
#define CONFIG_DECODERS ${CONFIG_DECODERS}
#define CONFIG_STATIC_MSVCRT ${CONFIG_STATIC_MSVCRT}
#define CONFIG_SPATIAL_RESAMPLING ${CONFIG_SPATIAL_RESAMPLING}
#define CONFIG_REALTIME_ONLY ${CONFIG_REALTIME_ONLY}
#define CONFIG_ONTHEFLY_BITPACKING ${CONFIG_ONTHEFLY_BITPACKING}
#define CONFIG_ERROR_CONCEALMENT ${CONFIG_ERROR_CONCEALMENT}
#define CONFIG_SHARED ${CONFIG_SHARED}
#define CONFIG_STATIC ${CONFIG_STATIC}
#define CONFIG_SMALL ${CONFIG_SMALL}
#define CONFIG_OS_SUPPORT ${CONFIG_OS_SUPPORT}
#define CONFIG_UNIT_TESTS ${CONFIG_UNIT_TESTS}
#define CONFIG_WEBM_IO ${CONFIG_WEBM_IO}
#define CONFIG_LIBYUV ${CONFIG_LIBYUV}
#define CONFIG_ACCOUNTING ${CONFIG_ACCOUNTING}
#define CONFIG_DECODE_PERF_TESTS ${CONFIG_DECODE_PERF_TESTS}
#define CONFIG_ENCODE_PERF_TESTS ${CONFIG_ENCODE_PERF_TESTS}
#define CONFIG_MULTI_RES_ENCODING ${CONFIG_MULTI_RES_ENCODING}
#define CONFIG_TEMPORAL_DENOISING ${CONFIG_TEMPORAL_DENOISING}
#define CONFIG_COEFFICIENT_RANGE_CHECKING ${CONFIG_COEFFICIENT_RANGE_CHECKING}
#define CONFIG_AOM_HIGHBITDEPTH ${CONFIG_AOM_HIGHBITDEPTH}
#define CONFIG_EXPERIMENTAL ${CONFIG_EXPERIMENTAL}
#define CONFIG_SIZE_LIMIT ${CONFIG_SIZE_LIMIT}
#define CONFIG_AOM_QM ${CONFIG_AOM_QM}
#define CONFIG_SPATIAL_SVC ${CONFIG_SPATIAL_SVC}
#define CONFIG_FP_MB_STATS ${CONFIG_FP_MB_STATS}
#define CONFIG_EMULATE_HARDWARE ${CONFIG_EMULATE_HARDWARE}
#define CONFIG_CLPF ${CONFIG_CLPF}
#define CONFIG_DERING ${CONFIG_DERING}
#define CONFIG_REF_MV ${CONFIG_REF_MV}
#define CONFIG_SUB8X8_MC ${CONFIG_SUB8X8_MC}
#define CONFIG_EXT_INTRA ${CONFIG_EXT_INTRA}
#define CONFIG_EXT_INTERP ${CONFIG_EXT_INTERP}
#define CONFIG_EXT_TX ${CONFIG_EXT_TX}
#define CONFIG_MOTION_VAR ${CONFIG_MOTION_VAR}
#define CONFIG_EXT_REFS ${CONFIG_EXT_REFS}
#define CONFIG_EXT_COMPOUND ${CONFIG_EXT_COMPOUND}
#define CONFIG_SUPERTX ${CONFIG_SUPERTX}
#define CONFIG_ANS ${CONFIG_ANS}
#define CONFIG_EC_MULTISYMBOL ${CONFIG_EC_MULTISYMBOL}
#define CONFIG_DAALA_EC ${CONFIG_DAALA_EC}
#define CONFIG_PARALLEL_DEBLOCKING ${CONFIG_PARALLEL_DEBLOCKING}
#define CONFIG_CB4X4 ${CONFIG_CB4X4}
#define CONFIG_PALETTE ${CONFIG_PALETTE}
#define CONFIG_FRAME_SIZE ${CONFIG_FRAME_SIZE}
#define CONFIG_FILTER_7BIT ${CONFIG_FILTER_7BIT}
#define CONFIG_DELTA_Q ${CONFIG_DELTA_Q}
#define CONFIG_ADAPT_SCAN ${CONFIG_ADAPT_SCAN}
#define CONFIG_BITSTREAM_DEBUG ${CONFIG_BITSTREAM_DEBUG}
#define CONFIG_TILE_GROUPS ${CONFIG_TILE_GROUPS}
#define CONFIG_EC_ADAPT ${CONFIG_EC_ADAPT}
#endif /* AOM_CONFIG_H */
