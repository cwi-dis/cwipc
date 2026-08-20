
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_tilefilter cwipc_tilefilter.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_tilefilter cwipc_util)

install(TARGETS cwipc_tilefilter RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_tilefilter COMMAND cwipc_tilefilter 0 ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/fixtures/input/pcl_frame1.ply ./pcl_frame1_tilefiltered.ply )
