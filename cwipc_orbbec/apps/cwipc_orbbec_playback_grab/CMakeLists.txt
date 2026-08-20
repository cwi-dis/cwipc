
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_orbbec_playback_grab cwipc_orbbec_playback_grab.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_orbbec_playback_grab cwipc_orbbec cwipc_util)

install(TARGETS cwipc_orbbec_playback_grab RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_orbbec_playback_grab COMMAND cwipc_orbbec_playback_grab 2 . ${PROJECT_SOURCE_DIR}/tests/fixtures/input/orbbec_recording/cameraconfig.json)
set_tests_properties(cwipc_orbbec_playback_grab PROPERTIES RESOURCE_LOCK orbbec_api_lock)
