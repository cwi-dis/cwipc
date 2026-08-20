
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_viewsynthetic cwipc_viewsynthetic.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_viewsynthetic cwipc_util)

install(TARGETS cwipc_viewsynthetic RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

