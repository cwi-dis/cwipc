
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_downsample cwipc_downsample.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_downsample cwipc_util)

install(TARGETS cwipc_downsample RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_downsample COMMAND cwipc_downsample 0.1 ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/fixtures/input/pcl_frame1.ply ./pcl_frame1_downsampled.ply )
