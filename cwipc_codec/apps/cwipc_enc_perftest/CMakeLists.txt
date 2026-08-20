
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_enc_perftest cwipc_enc_perftest.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_enc_perftest cwipc_codec ${PCL_LIBRARIES})

# install(TARGETS cwipc_enc_perftest RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_enc_perftest COMMAND cwipc_enc_perftest ${CMAKE_CURRENT_SOURCE_DIR}/../../tests/fixtures/input/pcl_frame1.ply )
