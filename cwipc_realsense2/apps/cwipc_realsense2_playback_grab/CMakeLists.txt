
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_realsense2_playback_grab cwipc_realsense2_playback_grab.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_realsense2_playback_grab cwipc_realsense2 cwipc_util)

# install(TARGETS cwipc_realsense2_grab RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

add_test(NAME cwipc_realsense2_playback_grab COMMAND cwipc_realsense2_playback_grab 2 . ${PROJECT_SOURCE_DIR}/tests/fixtures/input/realsense2_recording/cameraconfig.json)
set_tests_properties(cwipc_realsense2_playback_grab PROPERTIES RESOURCE_LOCK realsense_api_lock)
