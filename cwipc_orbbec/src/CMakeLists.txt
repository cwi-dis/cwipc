cmake_minimum_required(VERSION 3.16.0)
include(OrbbecExtensions)

add_library(cwipc_orbbec
	SHARED
	cwipc_orbbec.cpp
	OrbbecCamera.cpp
	OrbbecPlaybackCamera.cpp
	OrbbecCapture.cpp
	OrbbecPlaybackCapture.cpp
	OrbbecConfig.cpp
	cwipc_pcl_additions.cpp
)

target_sources(cwipc_orbbec
	PRIVATE
	"OrbbecBaseCamera.hpp"
	"OrbbecCamera.hpp"
	"OrbbecPlaybackCamera.hpp"
	"OrbbecBaseCapture.hpp"
	"OrbbecCapture.hpp"
	"OrbbecPlaybackCapture.hpp"
	"OrbbecConfig.hpp"
	"readerwriterqueue.h"
	"../include/cwipc_orbbec/api.h"
)

target_link_libraries(cwipc_orbbec PUBLIC cwipc_util)
target_link_libraries(cwipc_orbbec PRIVATE ${PCL_LIBRARIES})
target_link_libraries(cwipc_orbbec PRIVATE ob::OrbbecSDK)
target_link_libraries(cwipc_orbbec PRIVATE opencv_core opencv_imgproc)
target_link_libraries(cwipc_orbbec PRIVATE ZLIB::ZLIB)
target_link_libraries(cwipc_orbbec PRIVATE libjpeg-turbo::turbojpeg)
target_link_libraries(cwipc_orbbec PRIVATE nlohmann_json::nlohmann_json)

if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC" AND CMAKE_BUILD_TYPE MATCHES "Release")
	target_compile_options(cwipc_orbbec PRIVATE /Zi)

	# Tell linker to include symbol data
	set_target_properties(cwipc_orbbec PROPERTIES
	    LINK_FLAGS "/INCREMENTAL:NO /DEBUG /OPT:REF /OPT:ICF"
	)

	# Set file name & location
	set_target_properties(cwipc_orbbec PROPERTIES
	    COMPILE_PDB_NAME cwipc_orbbec
	    COMPILE_PDB_OUTPUT_DIR ${CMAKE_BINARY_DIR}
	)

	# Install pdb file
	install(DIRECTORY ${PROJECT_BINARY_DIR}/Release
		DESTINATION ${CMAKE_INSTALL_BINDIR}
		FILES_MATCHING PATTERN *.pdb
	)
endif()

set_target_properties(cwipc_orbbec PROPERTIES
	BUILD_RPATH ${OrbbecSDK_DIR}
)


if(WIN32)
	add_custom_command( TARGET cwipc_orbbec POST_BUILD
		COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_RUNTIME_DLLS:cwipc_orbbec> $<TARGET_FILE_DIR:cwipc_orbbec> 
		COMMAND_EXPAND_LISTS
	)
	copy_orbbec_extensions($<TARGET_FILE_DIR:cwipc_orbbec>)

	install(TARGETS cwipc_orbbec
		RUNTIME_DEPENDENCIES
		PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-" "k4a"
		POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
		EXPORT cwipc_orbbec
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION "include/cwipc_orbbec"
	)
	install_orbbec_extensions(${CMAKE_INSTALL_BINDIR})
elseif(APPLE)
	install(TARGETS cwipc_orbbec
		RUNTIME_DEPENDENCIES
		POST_EXCLUDE_REGEXES "/opt/homebrew" "/usr/local"
		EXPORT cwipc_orbbec
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		FRAMEWORK DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION "include/cwipc_orbbec"
	)
	install_orbbec_extensions(${CMAKE_INSTALL_LIBDIR})
else()
	install(TARGETS cwipc_orbbec
		RUNTIME_DEPENDENCIES
		POST_EXCLUDE_REGEXES "^/lib" "^/usr/lib"
		EXPORT cwipc_orbbec
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION "include/cwipc_orbbec"
	)
	install_orbbec_extensions(${CMAKE_INSTALL_LIBDIR})
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/cwipc_orbbec
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/CMakeFiles/cwipc_orbbec-config.cmake DESTINATION lib/cmake/cwipc_orbbec)

install(EXPORT cwipc_orbbec DESTINATION lib/cmake/cwipc_orbbec)

if(WIN32)
	# Copy the dependent DLLs that cmake/vcpkg have created
	#install(FILES $<TARGET_RUNTIME_DLLS:cwipc_orbbec> DESTINATION ${CMAKE_INSTALL_BINDIR})
	# Hack to also copy the zlib and zstd DLLs (which are not picked up by cmake/vcpkg)
	#install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/zlib1.dll DESTINATION ${CMAKE_INSTALL_BINDIR})
	#install(FILES ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>/zstd.dll DESTINATION ${CMAKE_INSTALL_BINDIR})
	# Copy the PDB file, if it exists
	install(FILES $<TARGET_PDB_FILE:cwipc_orbbec> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()
