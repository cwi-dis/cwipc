
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_generate cwipc_generate.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_generate cwipc_util)

install(TARGETS cwipc_generate RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_generate COMMAND cwipc_generate 2 . )
