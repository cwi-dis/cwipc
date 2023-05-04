cmake_policy(SET CMP0074 NEW)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
#
# Create venv if not done already
#
if(NOT VENV_DIR)
	#
	# Create venv for testing, and re-set Python3_EXECUTABLE to that Python.
	# We disable use of system-installed packages, because otherwise the testing of the scripts will fail.
	#
    get_filename_component(VENV_DIR "${CMAKE_BINARY_DIR}/venv" ABSOLUTE)
    execute_process(COMMAND ${Python3_EXECUTABLE} -m venv "${VENV_DIR}")
    set(ENV{VIRTUAL_ENV} "${VENV_DIR}")
    if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
    	file(TO_CMAKE_PATH ${Python3_EXECUTABLE} Python3_SYSTEM_EXECUTABLE)
        file(TO_CMAKE_PATH "${VENV_DIR}/Scripts" Python3_BINDIR)
        file(TO_CMAKE_PATH "${Python3_BINDIR}/python.exe" Python3_EXECUTABLE)
    else()
    	set(Python3_SYSTEM_EXECUTABLE ${Python3_EXECUTABLE})
        set(Python3_BINDIR "${VENV_DIR}/bin")
        set(Python3_EXECUTABLE "${Python3_BINDIR}/python")
    endif()
    execute_process(COMMAND ${Python3_EXECUTABLE} -m pip --quiet install --upgrade pip setuptools build wheel)
    message(STATUS "Created Python venv in ${VENV_DIR}")
endif()

#
# Add properties to tests that require Python, so the right paths are used for findping Python modules and DLLs and such
#
macro(cwipc_python_test)
	set(options)
	set(oneValueArgs)
	set(multiValueArgs TEST FIXTURES_REQUIRED)
	cmake_parse_arguments(MYARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	# Setup required fixtures, if any
	if(MYARGS_FIXTURES_REQUIRED)
		set_tests_properties(${MYARGS_TEST} PROPERTIES FIXTURES_REQUIRED ${MYARGS_FIXTURES_REQUIRED})
	endif()
	#
	# Prepend (not append) the directory where the just-built dynamic libraries live
	# to the runtime search path for dynamic libraries.
	# The environment variable to modify and the directory to add depend on the platform.
	#
	if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
		set_property(
			TEST ${MYARGS_TEST}
			APPEND PROPERTY ENVIRONMENT "DYLD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}:$ENV{DYLD_LIBRARY_PATH}"
		)
 	endif()
	if(${CMAKE_SYSTEM_NAME} MATCHES "Linux")
		set_property(
			TEST ${MYARGS_TEST}
			APPEND PROPERTY ENVIRONMENT "LD_LIBRARY_PATH=${CMAKE_LIBRARY_OUTPUT_DIRECTORY}:$ENV{LD_LIBRARY_PATH}"
		)
	endif()
	if(${CMAKE_SYSTEM_NAME} MATCHES "Windows")
		string(REPLACE ";" "\\;" prevpath "$ENV{PATH}")
		set_property(
			TEST ${MYARGS_TEST}
			APPEND PROPERTY ENVIRONMENT "PATH=${CMAKE_RUNTIME_OUTPUT_DIRECTORY}\\;${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>\\;${prevpath}"
		)
	endif()
  
endmacro()

#
# Build a Python wheel from a source directory
#
macro(cwipc_build_wheel)
	set(options)
	set(oneValueArgs SOURCEDIR WHEELDIR)
	set(multiValueArgs)
	cmake_parse_arguments(MYARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	execute_process(COMMAND ${Python3_EXECUTABLE} -m build --wheel --no-isolation --outdir ${MYARGS_WHEELDIR} ${MYARGS_SOURCEDIR} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endmacro()
#
# Install a wheel from the built wheels directory, and copy it to the installed wheel directory too.
#
macro(cwipc_install_wheel)
	set(options)
	set(oneValueArgs NAME WHEELDIR)
	set(multiValueArgs)
	cmake_parse_arguments(MYARGS "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	# The /.. in the DESTINATION is a trick to forestall double /python/python
	install(DIRECTORY ${MYARGS_WHEELDIR} DESTINATION ${CMAKE_PYWHEELS_INSTALL_DIRECTORY}/.. FILES_MATCHING PATTERN "${MYARGS_NAME}*.whl" )
	if(CWIPC_SKIP_PYTHON_INSTALL)
		message(STATUS "Skip install of Python wheel")
	else()
		message(STATUS "Installing Python wheel")
		install(CODE "execute_process(COMMAND \"${Python3_SYSTEM_EXECUTABLE}\" -m pip --quiet uninstall --yes ${MYARGS_NAME} WORKING_DIRECTORY \"${MYARGS_WHEELDIR}\" )")
		install(CODE "execute_process(COMMAND \"${Python3_SYSTEM_EXECUTABLE}\" -m pip --quiet install --find-links=. ${MYARGS_NAME} WORKING_DIRECTORY \"${MYARGS_WHEELDIR}\" )")
	endif()
endmacro()
