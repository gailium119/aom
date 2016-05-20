#!/bin/sh
## Copyright (c) 2016, Alliance for Open Media. All rights reserved
##
## This source code is subject to the terms of the BSD 2 Clause License and
## the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
## was not distributed with this source code in the LICENSE file, you can
## obtain it at www.aomedia.org/license/software. If the Alliance for Open
## Media Patent License 1.0 was not distributed with this source code in the
## PATENTS file, you can obtain it at www.aomedia.org/license/patent.
##
## This file tests the libaom simple_decoder example code. To add new tests to
## this file, do the following:
##   1. Write a shell function (this is your test).
##   2. Add the function to simple_decoder_tests (on a new line).
##
. $(dirname $0)/tools_common.sh

# Echo aomenc command line parameters allowing use of
# hantro_collage_w352h288.yuv as input.
yuv_input_hantro_collage() {
  echo ""${YUV_RAW_INPUT}"
       --width="${YUV_RAW_INPUT_WIDTH}"
       --height="${YUV_RAW_INPUT_HEIGHT}""
}

aomenc() {
  if [ "$(av1_encode_available)" = "yes" ]; then
    local readonly output=$1
    local readonly encoder="$(aom_tool_path aomenc)"

    eval "${encoder}" $(yuv_input_hantro_collage) \
      --codec=av1 \
      --ivf \
      --limit=5 \
      --output="${output}"

    if [ ! -e "${output}" ]; then
      elog "Output file does not exist."
      return 1
    fi
  fi
}

# Environment check: Make sure input is available:
simple_decoder_verify_environment() {
  return 0
}

# Runs simple_decoder using $1 as input file. $2 is the codec name, and is used
# solely to name the output file.
simple_decoder() {
  local decoder="${LIBAOM_BIN_PATH}/simple_decoder${AOM_TEST_EXE_SUFFIX}"
  local input_file="$1"
  local codec="$2"
  local output_file="${AOM_TEST_OUTPUT_DIR}/simple_decoder_${codec}.raw"

  if [ ! -x "${decoder}" ]; then
    elog "${decoder} does not exist or is not executable."
    return 1
  fi

  eval "${AOM_TEST_PREFIX}" "${decoder}" "${input_file}" "${output_file}" \
      ${devnull}

  [ -e "${output_file}" ] || return 1
}

simple_decoder_av1() {
  if [ "$(av1_decode_available)" = "yes" ]; then
    if [ ! -e "${AV1_IVF_FILE}" ]; then
      local file=${AOM_TEST_OUTPUT_DIR}/"test_encode.ivf"
      aomenc ${file}
      simple_decoder "${file}" av1 || return 1
    else
      simple_decoder "${AV1_IVF_FILE}" av1 || return 1
    fi

  fi
}

simple_decoder_tests="simple_decoder_av1"

run_tests simple_decoder_verify_environment "${simple_decoder_tests}"
