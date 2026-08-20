
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_util_install_check cwipc_util_install_check.cpp)
include_directories(
	"../../include"
)
target_link_libraries(cwipc_util_install_check cwipc_util)

install(TARGETS cwipc_util_install_check RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_util_install_check COMMAND cwipc_util_install_check )
