cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0072 NEW)
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

# Ensure all executable, dll/so and link-library outputs end up in the same directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PYWHEELS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/cwipc/python)
set(CMAKE_PYWHEELS_INSTALL_DIRECTORY ${CMAKE_INSTALL_DATADIR}/cwipc/python)
set(CMAKE_TESTDATA_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/cwipc/tests)
set(CMAKE_TESTDATA_INSTALL_DIRECTORY ${CMAKE_INSTALL_DATADIR}/cwipc/tests)

#
# Packages required by all submodules
#

# see here: https://github.com/PointCloudLibrary/pcl/issues/3680
# when this is fixed, we can remove the following 3 lines.
#if(NOT DEFINED CMAKE_SUPPRESS_DEVELOPER_WARNINGS)
#     set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE INTERNAL "No dev warnings")
#endif()

find_package(libjpeg-turbo REQUIRED)
find_package(PCL 1.10 REQUIRED COMPONENTS common io octree filters features)
find_package(Python3 REQUIRED COMPONENTS Interpreter)

#
# Optional packages, required by some submodules
#
find_package(k4a)
find_package(REALSENSE2)

add_subdirectory("cwipc_util")
add_subdirectory("cwipc_codec")
if(REALSENSE2_FOUND)
	add_subdirectory("cwipc_realsense2")
endif()
if(k4a_FOUND)
	add_subdirectory("cwipc_kinect")
endif()
