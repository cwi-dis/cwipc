# ======================================================
# Compiled by Fons @ CWI, Amsterdam for VRTogether
#
# Copyright (C) 2018 by CWI. All rights reserved.
# ======================================================
#
cmake_minimum_required(VERSION 3.16.0)


add_library(cwipc_kinect 
	SHARED 
	cwipc_kinect.cpp 
	K4ABaseCamera.cpp
	K4ACapture.cpp 
	K4ACamera.cpp 
	K4APlaybackCapture.cpp 
	K4APlaybackCamera.cpp 
	K4AConfig.cpp 
	cwipc_pcl_additions.cpp 
	${TINY_SRC}
)

target_sources(cwipc_kinect
	PRIVATE
	"K4AConfig.hpp"
	"K4ABaseCapture.hpp"
	"K4ABaseCamera.hpp"
	"K4ACapture.hpp"
	"K4ACamera.hpp"
	"K4APlaybackCapture.hpp"
	"K4APlaybackCamera.hpp"
	"readerwriterqueue.h"
	"atomicops.h"
	"../include/cwipc_kinect/api.h"
)

target_link_libraries(cwipc_kinect PUBLIC cwipc_util)
target_link_libraries(cwipc_kinect PRIVATE ${PCL_LIBRARIES})
target_link_libraries(cwipc_kinect PRIVATE k4a::k4a k4a::k4arecord)
if(CWIPC_WITH_KINECT_SKELETONS)
	target_link_libraries(cwipc_kinect PRIVATE k4a::k4abt)
endif()
target_link_libraries(cwipc_kinect PRIVATE opencv_core opencv_imgproc)
target_link_libraries(cwipc_kinect PRIVATE ZLIB::ZLIB) # Workaround for vcpkg
target_link_libraries(cwipc_kinect PRIVATE libjpeg-turbo::turbojpeg)
target_link_libraries(cwipc_kinect PRIVATE nlohmann_json::nlohmann_json)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
   target_compile_options(cwipc_kinect PRIVATE /Zi)

   # Tell linker to include symbol data
    set_target_properties(cwipc_kinect PROPERTIES 
        LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
    )

    # Set file name & location
    set_target_properties(cwipc_kinect PROPERTIES 
        COMPILE_PDB_NAME cwipc_kinect 
        COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
    )
	
	# Install pdb file
	install(DIRECTORY ${PROJECT_BINARY_DIR}/Release
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		FILES_MATCHING PATTERN *.pdb
	)
endif()

if(WIN32)
	install(TARGETS cwipc_kinect
	RUNTIME_DEPENDENCIES
	PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
	POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
	EXPORT cwipc_kinect
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION "include/cwipc_kinect"
	)
	# We also need to copy the depth engine
	install(FILES $<TARGET_FILE_DIR:cwipc_kinect>/depthengine_2_0.dll DESTINATION ${CMAKE_INSTALL_BINDIR})
else()
	install(TARGETS cwipc_kinect
	EXPORT cwipc_kinect
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION "include/cwipc_kinect"
	)
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/cwipc_kinect
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/CMakeFiles/cwipc_kinect-config.cmake DESTINATION lib/cmake/cwipc_kinect)

install(EXPORT cwipc_kinect DESTINATION lib/cmake/cwipc_kinect)

if(WIN32)
	# Copy the dependent DLLs that cmake/vcpkg have created
	#install(FILES $<TARGET_RUNTIME_DLLS:cwipc_kinect> DESTINATION ${CMAKE_INSTALL_BINDIR})
	# Hack to also copy the zlib and zstd DLLs (which are not picked up by cmake/vcpkg)
	#install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/zlib1.dll DESTINATION ${CMAKE_INSTALL_BINDIR})
	#install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/zstd.dll DESTINATION ${CMAKE_INSTALL_BINDIR})
	# Copy the PDB file, if it exists
	install(FILES $<TARGET_PDB_FILE:cwipc_kinect> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()
