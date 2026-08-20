# ======================================================
# Compiled by Fons @ CWI, Amsterdam for VRTogether
#
# Copyright (C) 2018 by CWI. All rights reserved.
# ======================================================
#
cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0074 NEW)

project(cwipc_realsense2)


list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMakeFiles)

option(BUILD_SHARED_LIBS "Build shared library" ON)

#
# Find cwipc_util
#
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	message(WARNING Building cwipc_realsense2 outside cwipc umbrella is not tested)
	find_package(cwipc_util REQUIRED)
	include_directories(${cwipc_util_INCLUDE_DIRS})
	link_directories(${cwipc_util_LIBRARY_DIRS})
endif()

#
# Find realsense (either with pkgconfig or with cmake)
#
find_package(REALSENSE2 REQUIRED)

# If we're on Linux os MacOS we need libusb
#
#if(NOT WIN32)
#	list(APPEND DEPENDENCIES m ${LIBUSB1_LIBRARIES})
#endif()

include_directories(include)

add_subdirectory(src)
add_subdirectory(apps/cwipc_realsense2_install_check)
add_subdirectory(apps/cwipc_realsense2_grab)
add_subdirectory(apps/cwipc_realsense2_playback_grab)
add_subdirectory(python)
add_subdirectory(tests)