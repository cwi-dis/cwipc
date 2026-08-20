
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_decode cwipc_decode.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_decode cwipc_codec ${PCL_LIBRARIES})

install(TARGETS cwipc_decode RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_decode COMMAND cwipc_decode ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/fixtures/compressed/pcl_frame1.cwicpc ./pcl_frame1.ply )
