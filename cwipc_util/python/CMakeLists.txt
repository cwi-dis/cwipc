cmake_minimum_required(VERSION 3.16.0)
cmake_policy(SET CMP0074 NEW)

# Setup venv-based Python

include(CwipcPythonSupport)

#
# It is expected that CMAKE_PYWHEELS_INSTALL_DIRECTORY and CMAKE_PYWHEELS_OUTPUT_DIRECTORY
# have been set already.
#

#
# Building.
#

# Create Python wheel in the staging area

cwipc_build_wheel(NAME "cwipc_util" SOURCEDIR ${CMAKE_CURRENT_SOURCE_DIR} WHEELDIR ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY})
file(COPY test_cwipc_util.py DESTINATION ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY})
file(COPY timing_tests.py DESTINATION ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY})
#
# Installation.
#

# Copy the whole Python staging subdirectory to the installation (so all scripts are available for inspection and testing)
# Also install the wheel into the current system Python.

install(FILES test_cwipc_util.py timing_tests.py DESTINATION ${CMAKE_PYWHEELS_INSTALL_DIRECTORY})
install(DIRECTORY examples DESTINATION  ${CMAKE_PYWHEELS_INSTALL_DIRECTORY})
cwipc_install_wheel(NAME "cwipc_util" WHEELDIR ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY})

# Install shell and bat scripts

install(PROGRAMS cwipc_pymodules_install.sh DESTINATION bin)
if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
	install(PROGRAMS cwipc_pymodules_install.bat DESTINATION bin)
	install(PROGRAMS cwipc_pymodules_install.ps1 DESTINATION bin)
endif()
if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
	install(PROGRAMS cwipc_macos_clearquarantine.sh DESTINATION bin)
endif()
install(PROGRAMS cwipc_tunnelproxy.sh DESTINATION bin)

# Run the unittests

add_test(NAME cwipc_util_python_tests
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY}/test_cwipc_util.py
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)
cwipc_python_test(TEST cwipc_util_python_tests FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME timing_tests
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY}/timing_tests.py
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
	)
cwipc_python_test(TEST timing_tests FIXTURES_REQUIRED venv_with_cwipc)

# Test the various scripts

# Some can be tested in isolation

add_test(NAME cwipc_view
	COMMAND ${Python3_BINDIR}/cwipc view --nodisplay --synthetic --count 100
	)
cwipc_python_test(TEST cwipc_view FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME cwipc_grab
	COMMAND ${Python3_BINDIR}/cwipc grab --synthetic --count 2 .
	)
cwipc_python_test(TEST cwipc_grab FIXTURES_REQUIRED venv_with_cwipc)

# Some can be tested in parallel
add_test(NAME cwipc_parallel
	COMMAND ${Python3_BINDIR}/cwipc parallel version -- version
)
cwipc_python_test(TEST cwipc_parallel FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(cwipc_parallel PROPERTIES RESOURCE_LOCK parallel_test)

add_test(NAME pipeline_forward_to_view
	COMMAND ${Python3_BINDIR}/cwipc parallel forward --synthetic --count 100 --nodrop -- view --netclient localhost:4303 --nodisplay
)
cwipc_python_test(TEST pipeline_forward_to_view FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(pipeline_forward_to_view PROPERTIES RESOURCE_LOCK parallel_test)

add_test(NAME pipeline_forward_to_view_passthrough
	COMMAND ${Python3_BINDIR}/cwipc parallel forward --synthetic --npoints 10000 --count 100 --nodrop --noencode -- view --netclient localhost:4303 --nodisplay --nodecode
)
cwipc_python_test(TEST pipeline_forward_to_view_passthrough FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(pipeline_forward_to_view_passthrough PROPERTIES RESOURCE_LOCK parallel_test)

add_test(NAME pipeline_forward_to_view_tiled
	COMMAND ${Python3_BINDIR}/cwipc parallel forward --synthetic --count 100 --nodrop -- view --netclient localhost:4303 --nodisplay
)
cwipc_python_test(TEST pipeline_forward_to_view_tiled FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(pipeline_forward_to_view_tiled PROPERTIES RESOURCE_LOCK parallel_test)

add_test(NAME pipeline_forward_to_netserver_to_view
	COMMAND ${Python3_BINDIR}/cwipc parallel netserver --oneshot -- view --netclient localhost:4303 --nodisplay -- forward --synthetic --forward localhost:4304 --nodrop --count 100
)
cwipc_python_test(TEST pipeline_forward_to_netserver_to_view FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(pipeline_forward_to_netserver_to_view PROPERTIES RESOURCE_LOCK parallel_test)

# Some cannot really be tested currently, let's at least import them

add_test(NAME cwipc_register
	COMMAND ${Python3_BINDIR}/cwipc register --version
	)
cwipc_python_test(TEST cwipc_register FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(cwipc_register PROPERTIES RESOURCE_LOCK realsense_api_lock orbbec_api_lock kinect_api_lock)

add_test(NAME cwipc_forward
	COMMAND ${Python3_BINDIR}/cwipc forward --version
	)
cwipc_python_test(TEST cwipc_forward FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME cwipc_toproxy
	COMMAND ${Python3_BINDIR}/cwipc toproxy --help
	)
cwipc_python_test(TEST cwipc_toproxy FIXTURES_REQUIRED venv_with_cwipc)

add_subdirectory(examples)