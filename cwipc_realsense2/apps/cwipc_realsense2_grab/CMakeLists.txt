
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_realsense2_grab cwipc_realsense2_grab.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_realsense2_grab cwipc_realsense2 cwipc_util)

install(TARGETS cwipc_realsense2_grab RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_realsense2_grab COMMAND cwipc_realsense2_grab 2 . auto)
set_tests_properties(cwipc_realsense2_grab PROPERTIES RESOURCE_LOCK realsense_api_lock)
if (NOT DEFINED ENV{CWIPC_TEST_HAVE_REALSENSE2_HARDWARE})
	set_tests_properties(cwipc_realsense2_grab PROPERTIES DISABLED TRUE)
endif()