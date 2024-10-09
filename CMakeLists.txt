cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0074 NEW)
cmake_policy(SET CMP0072 NEW)
#cmake_policy(SET CMP0135 NEW)


set(CWIPC_API_ONLY NO)
set(CWIPC_WITH_APPS NOT CWIPC_API_ONLY)
set(CWIPC_WITH_PYTHON NOT CWIPC_API_ONLY)
set(CWIPC_WITH_GUI NOT CWIPC_API_ONLY)
set(CWIPC_WITH_TESTS NOT CWIPC_API_ONLY)
set(CWIPC_WITH_DATA NOT CWIPC_API_ONLY)
set(CWIPC_WITH_CAPTURERS YES)
set(CWIPC_WITH_KINECT ${CWIPC_WITH_CAPTURERS})
set(CWIPC_WITH_REALSENSE ${CWIPC_WITH_CAPTURERS})
set(CWIPC_WITH_INSTALLER YES)

# Add extension directories (for things like Find<package>)
set(CMAKE_MODULE_PATH
	${CMAKE_CURRENT_LIST_DIR}/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_util/CMakeFiles
	${CMAKE_CURRENT_LIST_DIR}/cwipc_codec/CMakeFiles
	${CMAKE_MODULE_PATH}
	)
if(CWIPC_WITH_REALSENSE)
	set(CMAKE_MODULE_PATH
		${CMAKE_CURRENT_LIST_DIR}/cwipc_realsense2/CMakeFiles
		${CMAKE_MODULE_PATH}
		)
endif()
if(CWIPC_WITH_KINECT)
	set(CMAKE_MODULE_PATH
		${CMAKE_CURRENT_LIST_DIR}/cwipc_kinect/CMakeFiles
		${CMAKE_MODULE_PATH}
		)
endif()

# Get version from git if not specified on the command line
if(NOT CWIPC_VERSION)
	include(GetGitVersion)
	get_git_version(VERSION_VAR CWIPC_VERSION)
endif()
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
set(CMAKE_REGISTRATIONDATA_INSTALL_DIRECTORY ${CMAKE_INSTALL_DATADIR}/cwipc/registration)
set(CMAKE_SCRIPTS_INSTALL_DIRECTORY ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

#
# Ensure we use the correct RPATH in installed binaries.
# This also works in the build tree, because we use the staging area to
# build all executables and dynamic libraries and it has the
# same layout as the installed tree
#
if(APPLE)
	set(CMAKE_INSTALL_RPATH "@loader_path/../lib;@loader_path/../../lib")
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
elseif(UNIX)
	set(CMAKE_INSTALL_RPATH "$ORIGIN/../lib;$ORIGIN/../../lib")
	set(CMAKE_SKIP_BUILD_RPATH FALSE)
	set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()

if(CWIPC_WITH_PYTHON)
	include(CwipcPythonSupport)
	#
	# Ensure we clean out the Python wheels directory before installing 
	#
	GNUInstallDirs_get_absolute_install_dir(WHEELDEST CMAKE_PYWHEELS_INSTALL_DIRECTORY WHEELDEST)
	install(CODE "message(\"-- Remove ${WHEELDEST}\")")
	install(CODE "file(REMOVE_RECURSE \"${WHEELDEST}\")")

	# The upper limit on the Python version is caused by opencv and open3d,
	# which can lag behind some times. As soon as those packages are available
	# for a new release of Python we can update this version.
	#
	find_package(Python3 3.9...<3.12 REQUIRED COMPONENTS Interpreter)

	#
	# Now we can setup the venv.
	#
	cwipc_setup_python_venv()
endif()

#
# Packages required by all submodules
#
if(CMAKE_SYSTEM_NAME STREQUAL Linux)
	message(WARNING "Need to find jpeg-turbo")
	include(FindPkgConfig)
	pkg_check_modules(libturbojpeg REQUIRED libturbojpeg)
	pkg_get_variable(libturbojpeg_LIBDIR libturbojpeg libdir)
	add_library(libjpeg-turbo::jpeg SHARED IMPORTED)
	add_library(libjpeg-turbo::turbojpeg SHARED IMPORTED)
	
	set_target_properties(libjpeg-turbo::jpeg PROPERTIES
		IMPORTED_LOCATION "${libturbojpeg_LIBDIR}/libjpeg.so"
	)
	set_target_properties(libjpeg-turbo::turbojpeg PROPERTIES
		IMPORTED_LOCATION "${libturbojpeg_LIBDIR}/libturbojpeg.so"
	)
	
else()
	find_package(libjpeg-turbo REQUIRED)
endif()
find_package(PCL REQUIRED COMPONENTS common io octree filters features geometry kdtree)


#
# Get the nlohmann/json package
#
# First see if it is already installed.

find_package(nlohmann_json 3.11.2)

if(NOT nlohmann_json_FOUND)
	# If it is not installed already we download a private copy.
	set(FETCHCONTENT_QUIET OFF)
	include(FetchContent)
	
	FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.11.2/json.tar.xz)
	FetchContent_MakeAvailable(json)
endif()

#
# Optional packages, required by some submodules
#
if(CWIPC_WITH_KINECT)
	find_package(k4a)
endif()
if(CWIPC_WITH_REALSENSE)
	find_package(REALSENSE2)
endif()

add_subdirectory("cwipc_util")
add_subdirectory("cwipc_codec")
if(REALSENSE2_FOUND)
	add_subdirectory("cwipc_realsense2")
endif()
if(k4a_FOUND)
	add_subdirectory("cwipc_kinect")
endif()
if(CWIPC_WITH_APPS)
# Install scripts to help installing prerequisites
	install(DIRECTORY scripts DESTINATION ${CMAKE_SCRIPTS_INSTALL_DIRECTORY})
endif()
if(CWIPC_WITH_INSTALLER)
	# Creating installers
	include(CwipcInstallers)
endif()