##
## Copyright (c) 2016, Alliance for Open Media. All rights reserved
##
## This source code is subject to the terms of the BSD 2 Clause License and
## the Alliance for Open Media Patent License 1.0. If the BSD 2 Clause License
## was not distributed with this source code in the LICENSE file, you can
## obtain it at www.aomedia.org/license/software. If the Alliance for Open
## Media Patent License 1.0 was not distributed with this source code in the
## PATENTS file, you can obtain it at www.aomedia.org/license/patent.
##
cmake_minimum_required(VERSION 3.2)

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)

# Strings used to cache failed C/CXX flags.
set(AOM_FAILED_C_FLAGS)
set(AOM_FAILED_CXX_FLAGS)

# Checks C compiler for support of $c_flag. Adds $c_flag to $CMAKE_C_FLAGS when
# the compile test passes. Caches $c_flag in $AOM_FAILED_C_FLAGS when the test
# fails.
function (add_c_flag_if_supported c_flag)
  unset(C_FLAG_FOUND CACHE)
  string(FIND "${CMAKE_C_FLAGS}" "${c_flag}" C_FLAG_FOUND)
  unset(C_FLAG_FAILED CACHE)
  string(FIND "${AOM_FAILED_C_FLAGS}" "${c_flag}" C_FLAG_FAILED)

  if (${C_FLAG_FOUND} EQUAL -1 AND ${C_FLAG_FAILED} EQUAL -1)
    unset(C_FLAG_SUPPORTED CACHE)
    message("Checking C compiler flag support for: " ${c_flag})
    check_c_compiler_flag("${c_flag}" C_FLAG_SUPPORTED)
    if (C_FLAG_SUPPORTED)
      set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${c_flag}" CACHE STRING "" FORCE)
    else ()
      set(AOM_FAILED_C_FLAGS "${AOM_FAILED_C_FLAGS} ${c_flag}" CACHE STRING ""
          FORCE)
    endif ()
  endif ()
endfunction ()

# Checks C++ compiler for support of $cxx_flag. Adds $cxx_flag to
# $CMAKE_CXX_FLAGS when the compile test passes. Caches $c_flag in
# $AOM_FAILED_CXX_FLAGS when the test fails.
function (add_cxx_flag_if_supported cxx_flag)
  unset(CXX_FLAG_FOUND CACHE)
  string(FIND "${CMAKE_CXX_FLAGS}" "${cxx_flag}" CXX_FLAG_FOUND)
  unset(CXX_FLAG_FAILED CACHE)
  string(FIND "${AOM_FAILED_CXX_FLAGS}" "${cxx_flag}" CXX_FLAG_FAILED)

  if (${CXX_FLAG_FOUND} EQUAL -1 AND ${CXX_FLAG_FAILED} EQUAL -1)
    unset(CXX_FLAG_SUPPORTED CACHE)
    message("Checking CXX compiler flag support for: " ${cxx_flag})
    check_cxx_compiler_flag("${cxx_flag}" CXX_FLAG_SUPPORTED)
    if (CXX_FLAG_SUPPORTED)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${cxx_flag}" CACHE STRING ""
          FORCE)
    else()
      set(AOM_FAILED_CXX_FLAGS "${AOM_FAILED_CXX_FLAGS} ${cxx_flag}" CACHE
          STRING "" FORCE)
    endif ()
  endif ()
endfunction ()

# Convenience method for adding a flag to both the C and C++ compiler command
# lines.
function (add_compiler_flag_if_supported flag)
  add_c_flag_if_supported(${flag})
  add_cxx_flag_if_supported(${flag})
endfunction ()

# Checks C compiler for support of $c_flag and terminates generation when
# support is not present.
function (require_c_flag c_flag)
  unset(C_FLAG_FOUND CACHE)
  string(FIND "${CMAKE_C_FLAGS}" "${c_flag}" C_FLAG_FOUND)

  if (${C_FLAG_FOUND} EQUAL -1)
    unset(HAVE_C_FLAG CACHE)
    message("Checking C compiler flag support for: " ${c_flag})
    check_c_compiler_flag("${c_flag}" HAVE_C_FLAG)
    if (NOT HAVE_C_FLAG)
      message(FATAL_ERROR
              "${PROJECT_NAME} requires support for C flag: ${c_flag}.")
    endif ()
    set(CMAKE_C_FLAGS "${c_flag} ${CMAKE_C_FLAGS}" CACHE STRING "" FORCE)
  endif ()
endfunction ()

# Checks CXX compiler for support of $cxx_flag and terminates generation when
# support is not present.
function (require_cxx_flag cxx_flag)
  unset(CXX_FLAG_FOUND CACHE)
  string(FIND "${CMAKE_CXX_FLAGS}" "${cxx_flag}" CXX_FLAG_FOUND)

  if (${CXX_FLAG_FOUND} EQUAL -1)
    unset(HAVE_CXX_FLAG CACHE)
    message("Checking CXX compiler flag support for: " ${cxx_flag})
    check_cxx_compiler_flag("${cxx_flag}" HAVE_CXX_FLAG)
    if (NOT HAVE_CXX_FLAG)
      message(FATAL_ERROR
              "${PROJECT_NAME} requires support for CXX flag: ${cxx_flag}.")
    endif ()
    set(CMAKE_CXX_FLAGS "${cxx_flag} ${CMAKE_CXX_FLAGS}" CACHE STRING "" FORCE)
  endif ()
endfunction ()

# Checks only for support of $flag by both the C and CXX compilers. Terminates
# generation when support is not present in both compilers.
function (require_flag flag)
  require_c_flag(${flag})
  require_cxx_flag(${flag})
endfunction ()

# Checks only non-MSVC targets for support of $c_flag and terminates generation
# when support is not present.
function (require_c_flag_nomsvc c_flag)
  if (NOT MSVC)
    require_c_flag(${c_flag})
  endif ()
endfunction ()

# Checks only non-MSVC targets for support of $cxx_flag and terminates
# generation when support is not present.
function (require_cxx_flag_nomsvc cxx_flag)
  if (NOT MSVC)
    require_cxx_flag(${cxx_flag})
  endif ()
endfunction ()

# Checks only non-MSVC targets for support of $flag by both the C and CXX
# compilers. Terminates generation when support is not present in both
# compilers.
function (require_flag_nomsvc flag)
  require_c_flag_nomsvc(${flag})
  require_cxx_flag_nomsvc(${flag})
endfunction ()

# Adds $preproc_def to C compiler command line (as -D$preproc_def) if not
# already present.
function (add_c_preproc_definition preproc_def)
  unset(PREPROC_DEF_FOUND CACHE)
  string(FIND "${CMAKE_C_FLAGS}" "${preproc_def}" PREPROC_DEF_FOUND)

  if (${PREPROC_DEF_FOUND} EQUAL -1)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -D${preproc_def}" CACHE STRING ""
        FORCE)
  endif ()
endfunction ()

# Adds $preproc_def to CXX compiler command line (as -D$preproc_def) if not
# already present.
function (add_cxx_preproc_definition preproc_def)
  unset(PREPROC_DEF_FOUND CACHE)
  string(FIND "${CMAKE_CXX_FLAGS}" "${preproc_def}" PREPROC_DEF_FOUND)

  if (${PREPROC_DEF_FOUND} EQUAL -1)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${preproc_def}" CACHE STRING ""
        FORCE)
  endif ()
endfunction ()

function (add_preproc_definition preproc_def)
  add_c_preproc_definition(${preproc_def})
  add_cxx_preproc_definition(${preproc_def})
endfunction ()
