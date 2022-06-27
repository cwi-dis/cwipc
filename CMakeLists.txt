cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0072 NEW)

# Add extension directories (for things like Find<package>)
set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_LIST_DIR}/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_util/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_codec/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_realsense2/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_kinect/CMakeFiles
	${CMAKE_MODULE_PATH}
	)

# Get version from git
include(GetGitVersion)
get_git_version(VERSION_VAR CWIPC_VERSION)
if(CWIPC_VERSION)
	message(STATUS "CWIPC_VERSION=${CWIPC_VERSION}")
	add_compile_definitions(CWIPC_VERSION=${CWIPC_VERSION})
	set(ENV{CWIPC_VERSION} ${CWIPC_VERSION})
	# Remove patch from version before we pass it to the project VERSION argument (it doesn't understand)
	string(REGEX REPLACE "\\+.*" "" CWIPC_BASE_VERSION ${CWIPC_VERSION})
endif()

project(cwipc VERSION ${CWIPC_BASE_VERSION})

include(CTest)
include(GNUInstallDirs)

# Ensure all executable, dll/so and link-library outputs end up in the same directories
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_PYWHEELS_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/cwipc/python)
set(CMAKE_PYWHEELS_INSTALL_DIRECTORY ${CMAKE_INSTALL_DATADIR}/cwipc/python)
set(CMAKE_TESTDATA_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/share/cwipc/tests)
set(CMAKE_TESTDATA_INSTALL_DIRECTORY ${CMAKE_INSTALL_DATADIR}/cwipc/tests)
set(CMAKE_SCRIPTS_INSTALL_DIRECTORY ${CMAKE_INSTALL_DATADIR}/cwipc)

# Ensure we use the correct RPATH in installed binaries.
# This also works in the build tree, because we use the staging area to
# build all executables and dynamic libraries and it has the
# same layout as the installed tree
if(APPLE)
	set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
elseif(UNIX)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib")
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

#
# Packages required by all submodules
#

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

# Install scripts to help installing prerequisites
install(DIRECTORY scripts DESTINATION ${CMAKE_SCRIPTS_INSTALL_DIRECTORY})

# Creating installers
include(CwipcInstallers)
