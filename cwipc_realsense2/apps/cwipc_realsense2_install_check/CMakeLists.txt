
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_realsense2_install_check cwipc_realsense2_install_check.cpp)
include_directories(
	"../../include"
)
target_link_libraries(cwipc_realsense2_install_check cwipc_realsense2)

install(TARGETS cwipc_realsense2_install_check RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_realsense2_install_check COMMAND cwipc_realsense2_install_check )
set_tests_properties(cwipc_realsense2_install_check PROPERTIES RESOURCE_LOCK realsense_api_lock)
