
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_kinect_playback_grab cwipc_kinect_playback_grab.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_kinect_playback_grab cwipc_kinect cwipc_util)

install(TARGETS cwipc_kinect_playback_grab RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_kinect_playback_grab COMMAND cwipc_kinect_playback_grab 2 . ${PROJECT_SOURCE_DIR}/tests/fixtures/input/kinect_recording/cameraconfig.json)
