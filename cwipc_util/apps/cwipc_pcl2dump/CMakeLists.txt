
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_pcl2dump cwipc_pcl2dump.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_pcl2dump cwipc_util)

install(TARGETS cwipc_pcl2dump RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_pcl2dump COMMAND cwipc_pcl2dump ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/fixtures/input/pcl_frame1.ply ./pcl_frame1.cwipcdump )
