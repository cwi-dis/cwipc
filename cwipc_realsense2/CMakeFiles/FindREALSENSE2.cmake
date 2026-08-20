# ======================================================
# Compiled by Fons @ CWI, Amsterdam for VRTogether
#
# This code serves to find Intel RealSense SDK software
# Running it defines:
#	REALSENSE2_FOUND
#	REALSENSE2_INCLUDE_DIRS
#	REALSENSE2_LIBRARIES
#
# Copyright (C) 2018 by CWI. All rights reserved.
# ======================================================

set(_REALSENSE2_SEARCHES)

# Search REALSENSE2_ROOT first if it is set.

if(REALSENSE2_ROOT)
  set(_REALSENSE2_SEARCH_ROOT PATHS ${REALSENSE2_ROOT} NO_DEFAULT_PATH)
  list(APPEND _REALSENSE2_SEARCHES _REALSENSE2_SEARCH_ROOT)
endif()

# On Windows, we add the default install location

if(WIN32)
  list(APPEND _REALSENSE2_SEARCHES "C:/Program Files (x86)/Intel RealSense SDK 2.0/")
#  set(CMAKE_FIND_LIBRARY_SUFFIXES .dll ${CMAKE_FIND_LIBRARY_SUFFIXES})
endif()

# Try each search configuration.
find_path(REALSENSE2_INCLUDE_DIR NAMES librealsense2/rs.hpp PATHS ${_REALSENSE2_SEARCHES} PATH_SUFFIXES include)
find_library(REALSENSE2_LIB NAMES realsense2 PATHS ${_REALSENSE2_SEARCHES} PATH_SUFFIXES lib/x64)


include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(REALSENSE2 DEFAULT_MSG REALSENSE2_LIB REALSENSE2_INCLUDE_DIR)

if(REALSENSE2_FOUND)
  set(REALSENSE2_INCLUDE_DIRS ${REALSENSE2_INCLUDE_DIR})
  set(REALSENSE2_LIBRARIES ${REALSENSE2_LIB})
  
  if(NOT TARGET REALSENSE2::REALSENSE2)
    add_library(REALSENSE2::REALSENSE2 UNKNOWN IMPORTED)
    set_target_properties(REALSENSE2::REALSENSE2 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${REALSENSE2_INCLUDE_DIRS}"
      )
    set_property(TARGET REALSENSE2::REALSENSE2 APPEND PROPERTY
      IMPORTED_LOCATION "${REALSENSE2_LIB}"
      )
  endif()
endif()

