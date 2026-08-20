# ======================================================
# Compiled by Fons @ CWI, Amsterdam for VRTogether
#
# Copyright (C) 2018 by CWI. All rights reserved.
# ======================================================
#
cmake_minimum_required(VERSION 3.16.0)

project(cwipc_kinect)

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMakeFiles)

option(BUILD_SHARED_LIBS "Build shared library" ON)

#
# Find cwipc_util
#
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	message(WARNING Building cwipc_kinect outside cwipc umbrella is not tested)
	find_package(cwipc_util REQUIRED)
	include_directories(${cwipc_util_INCLUDE_DIRS})
	link_directories(${cwipc_util_LIBRARY_DIRS})
endif()

#
# Find Azure Kinect SDK (either with pkgconfig or with cmake)
#
find_package(k4a REQUIRED)
find_package(k4abt)

# We need OpenCV for some image filtering

find_package( OpenCV REQUIRED
	PATHS "C:/opencv" 
	PATH_SUFFIXES build 
	)
include_directories( ${OpenCV_INCLUDE_DIRS} )

# See if we have k4abt support
if( k4abt_FOUND)
	add_compile_definitions(CWIPC_WITH_KINECT_SKELETON)
	set(CWIPC_WITH_KINECT_SKELETON)
endif()

# If we are on Linux we should skip tests if we have no X11 server
if(NOT WIN32 AND NOT APPLE)
	if(NOT DEFINED ENV{DISPLAY})
		set(BUILDING_ON_HEADLESS_UNIX on)
	endif()
endif()

include_directories(include)

add_subdirectory(src)
add_subdirectory(apps/cwipc_kinect_install_check)
add_subdirectory(apps/cwipc_kinect_grab)
add_subdirectory(apps/cwipc_kinect_playback_grab)
add_subdirectory(python)
add_subdirectory(tests)