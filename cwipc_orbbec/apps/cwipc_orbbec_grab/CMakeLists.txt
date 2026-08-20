
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_orbbec_grab cwipc_orbbec_grab.cpp)
include_directories(
	"../../include"
	${PCL_INCLUDE_DIRS}
)
target_link_libraries(cwipc_orbbec_grab cwipc_orbbec cwipc_util)

install(TARGETS cwipc_orbbec_grab RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/cwipc)

add_test(NAME cwipc_orbbec_grab COMMAND cwipc_orbbec_grab 2 . auto)
set_tests_properties(cwipc_orbbec_grab PROPERTIES RESOURCE_LOCK orbbec_api_lock)
if(NOT DEFINED ENV{CWIPC_TEST_HAVE_ORBBEC_HARDWARE})
	set_tests_properties(cwipc_orbbec_grab PROPERTIES DISABLED TRUE)
endif()
