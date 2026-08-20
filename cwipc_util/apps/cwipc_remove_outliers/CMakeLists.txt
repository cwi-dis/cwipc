
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_remove_outliers cwipc_remove_outliers.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_remove_outliers cwipc_util)

install(TARGETS cwipc_remove_outliers RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_remove_outliers COMMAND cwipc_remove_outliers 30 1.5 1 ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/fixtures/input/pcl_frame1.ply ./pcl_frame1_cleaned.ply )
