# ======================================================
# Compiled by Fons @ CWI, Amsterdam for VRTogether
#
# Copyright (C) 2018 by CWI. All rights reserved.
# ======================================================
#
cmake_minimum_required(VERSION 3.16.0)


add_library(cwipc_realsense2 
	SHARED 
	cwipc_realsense2.cpp 
	RS2BaseCamera.cpp
	RS2Capture.cpp 
	RS2Camera.cpp 
	RS2PlaybackCapture.cpp 
	RS2PlaybackCamera.cpp 
	RS2Config.cpp 
	cwipc_pcl_additions.cpp 
	${TINY_SRC}
)

target_sources(cwipc_realsense2
	PRIVATE
	"RS2Config.hpp"
	"RS2BaseCapture.hpp"
	"RS2BaseCamera.hpp"
	"RS2Capture.hpp"
	"RS2Camera.hpp"
	"RS2PlaybackCapture.hpp"
	"RS2PlaybackCamera.hpp"
	"../include/cwipc_realsense2/api.h"
)

target_link_libraries(cwipc_realsense2 PUBLIC cwipc_util)
target_link_libraries(cwipc_realsense2 PRIVATE ${PCL_LIBRARIES} REALSENSE2::REALSENSE2)
target_link_libraries(cwipc_realsense2 PRIVATE nlohmann_json::nlohmann_json)

if(WIN32)
	install(TARGETS cwipc_realsense2
	RUNTIME_DEPENDENCIES
	PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
	POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
	EXPORT cwipc_realsense2
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION "include/cwipc_realsense2"
	)
else()
	install(TARGETS cwipc_realsense2
	EXPORT cwipc_realsense2
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION "include/cwipc_realsense2"
	)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/cwipc_realsense2
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/CMakeFiles/cwipc_realsense2-config.cmake DESTINATION lib/cmake/cwipc_realsense2)

install(EXPORT cwipc_realsense2 DESTINATION lib/cmake/cwipc_realsense2)

if(WIN32)
	# Copy the dependent DLLs that cmake/vcpkg have created
	#install(FILES $<TARGET_RUNTIME_DLLS:cwipc_realsense2> DESTINATION ${CMAKE_INSTALL_BINDIR})
	# Copy the PDB file, if it exists
	install(FILES $<TARGET_PDB_FILE:cwipc_realsense2> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()
