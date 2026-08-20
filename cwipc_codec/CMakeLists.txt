
cmake_minimum_required(VERSION 3.16.0)
project(cwipc_codec)


#
# Find cwipc_util
#
if( CMAKE_SOURCE_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR )
	message(WARNING Building cwipc_codec outside cwipc umbrella is not tested)
	find_package(cwipc_util REQUIRED)
	include_directories(${cwipc_util_INCLUDE_DIRS})
	link_directories(${cwipc_util_LIBRARY_DIRS})
endif()

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_LIST_DIR}/CMakeFiles)

add_subdirectory(cloud_codec_v2)
add_subdirectory(src)
if(CWIPC_WITH_TESTS)
	#add_subdirectory("apps/evaluate_compression")
	add_subdirectory(apps/cwipc_codec_install_check)
	add_subdirectory(apps/cwipc_encode)
	add_subdirectory(apps/cwipc_decode)
	add_subdirectory(apps/cwipc_enc_perftest)
endif()
if(CWIPC_WITH_PYTHON)
	add_subdirectory(python)
endif()
if(CWIPC_WITH_TESTS)
	add_subdirectory(tests)
endif()
