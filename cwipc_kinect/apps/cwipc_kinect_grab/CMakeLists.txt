
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_kinect_grab cwipc_kinect_grab.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_kinect_grab cwipc_kinect cwipc_util)

install(TARGETS cwipc_kinect_grab RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_kinect_grab COMMAND cwipc_kinect_grab 2 . auto)
set_tests_properties(cwipc_kinect_grab PROPERTIES RESOURCE_LOCK kinect_api_lock)
if (NOT DEFINED ENV{CWIPC_TEST_HAVE_KINECT_HARDWARE})
	set_tests_properties(cwipc_kinect_grab PROPERTIES DISABLED TRUE)
endif()