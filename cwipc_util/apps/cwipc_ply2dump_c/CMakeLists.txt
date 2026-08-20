
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_ply2dump_c cwipc_ply2dump_c.c)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_ply2dump_c cwipc_util)

install(TARGETS cwipc_ply2dump_c RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_ply2dump_c COMMAND cwipc_ply2dump_c ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/fixtures/input/pcl_frame1.ply ./pcl_frame1.cwipcdump )
