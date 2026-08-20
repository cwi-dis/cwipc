
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_codec_install_check cwipc_codec_install_check.cpp)
include_directories(
	"../../include"
)
target_link_libraries(cwipc_codec_install_check cwipc_codec)

install(TARGETS cwipc_codec_install_check RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_codec_install_check COMMAND cwipc_codec_install_check )
