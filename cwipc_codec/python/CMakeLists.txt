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

cwipc_build_wheel(NAME "cwipc_codec" SOURCEDIR ${CMAKE_CURRENT_SOURCE_DIR} WHEELDIR ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY})
file(COPY test_cwipc_codec.py DESTINATION ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY})

#
# Installation.
#

# Copy the whole Python staging subdirectory to the installation (so all scripts are available for inspection and testing)
# Also install the wheel into the current system Python.

install(FILES test_cwipc_codec.py DESTINATION ${CMAKE_PYWHEELS_INSTALL_DIRECTORY})
cwipc_install_wheel(NAME "cwipc_codec" WHEELDIR ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY})


add_test(NAME cwipc_codec_python_tests
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_PYWHEELS_OUTPUT_DIRECTORY}/test_cwipc_codec.py
	WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
)
cwipc_python_test(TEST cwipc_codec_python_tests FIXTURES_REQUIRED venv_with_cwipc)
