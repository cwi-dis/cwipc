cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0074 NEW)

project(cwipc)

include(CTest)
include(GNUInstallDirs)

# Add extension directories (for things like Find<package>)
set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_LIST_DIR}/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_util/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_codec/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_realsense2/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_kinect/CMakeFiles
	${CMAKE_MODULE_PATH}
	)
# Workaround for older cmake versions on Windows? Need to investigate. xxxjack.
# set(OLD_WINDOWS_CMAKE_WORKAROUND TRUE)
if(OLD_WINDOWS_CMAKE_WORKAROUND)
	# Ensure all executable, dll/so and link-library outputs end up in the same directories
	set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin/${CMAKE_BUILD_TYPE})
	set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
	set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib/${CMAKE_BUILD_TYPE})
endif()

#
# Packages required by all submodules
#

# see here: https://github.com/PointCloudLibrary/pcl/issues/3680
# when this is fixed, we can remove the following 3 lines.
#if(NOT DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
#     set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
#endif()

find_package(PCL 1.10 REQUIRED COMPONENTS common io octree filters features)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
#
# Packages required by submodules cwipc_util and cwipc_codec (which are non-optional
#
# Disabled for now (will be found by submodules) find_package(JPEG_Turbo REQUIRED)
#find_package(Boost 1.47.0 ${QUIET_FLAG} REQUIRED COMPONENTS system filesystem thread date_time iostreams chrono program_options)
#find_package(Eigen 4.3 ${QUIET_FLAG} REQUIRED)

#
# Optional packages, required by some submodules
#
find_package(KINECT)
find_package(REALSENSE2)

add_subdirectory("cwipc_util")
add_subdirectory("cwipc_codec")
if(REALSENSE2_FOUND)
	add_subdirectory("cwipc_realsense2")
endif()
if(KINECT_FOUND)
	add_subdirectory("cwipc_kinect")
endif()
