# Workaround for finding older libjpeg-turbo versions that provide pkgconfig
# but not cmake configuration files. Specifically for Ubuntu 20.04.

#.rst:
# FindJPEG_Turbo
# --------
#
# Find JPEG_Turbo
#
# Find the JPEG_Turbo includes and library This module defines
#
# ::
#
#   libjpeg_turbo_INCLUDE_DIR, where to find jpeglib.h, etc.
#   libjpeg_turbo_LIBRARIES, the libraries needed to use libjpeg_turbo.
#   libjpeg_turbo_FOUND, If false, do not try to use JPEG_Turbo.
#
# also defined, but not for general use are
#
# ::
#
#   JPEG_Turbo_LIBRARY, where to find the JPEG_Turbo library.

#=============================================================================
# Copyright 2001-2009 Kitware, Inc.
#
# Distributed under the OSI-approved BSD License (the "License");
# see accompanying file Copyright.txt for details.
#
# This software is distributed WITHOUT ANY WARRANTY; without even the
# implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
# See the License for more information.
#=============================================================================
# (To distribute this file outside of CMake, substitute the full
#  License text for the above reference.)

find_path(libjpeg_turbo_INCLUDE_DIR turbojpeg.h)

# codec want libjpeg-compatible library, not turbojpeg library:
find_library(libjpeg_turbo_Compat_LIBRARY NAMES jpeg )
find_library(libjpeg_turbo_LIBRARY NAMES turbojpeg )

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(libjpeg_turbo DEFAULT_MSG libjpeg_turbo_LIBRARY libjpeg_turbo_Compat_LIBRARY libjpeg_turbo_INCLUDE_DIR)

if(libjpeg_turbo_FOUND)

  if(NOT TARGET libjpeg-turbo::turbojpeg)
    add_library(libjpeg-turbo::turbojpeg UNKNOWN IMPORTED)
    set_target_properties(libjpeg-turbo::turbojpeg PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION ${libjpeg_turbo_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES "${libjpeg_turbo_INCLUDE_DIR}"
    )
  endif()

  if(NOT TARGET libjpeg-turbo::jpeg)
    add_library(libjpeg-turbo::jpeg UNKNOWN IMPORTED)
    set_target_properties(libjpeg-turbo::jpeg PROPERTIES
      IMPORTED_LINK_INTERFACE_LANGUAGES "C"
      IMPORTED_LOCATION ${libjpeg_turbo_Compat_LIBRARY}
      INTERFACE_INCLUDE_DIRECTORIES "${libjpeg_turbo_INCLUDE_DIR}"
    )
  endif()

  set(libjpeg_turbo_LIBRARIES ${libjpeg_turbo_LIBRARY} ${libjpeg_turbo_Compat_LIBRARY})
  message(STATUS "-- Found libjpeg_turbo: " ${libjpeg_turbo_LIBRARIES})
else()
  message(WARNING "-- libjpeg_turbo not found")
endif()

mark_as_advanced(libjpeg_turbo_LIBRARY libjpeg_turbo_INCLUDE_DIR libjpeg_turbo_Compat_LIBRARY)
