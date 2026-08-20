cmake_minimum_required(VERSION 3.16.0)

add_library(cwipc_util SHARED 
	cwipc_util.cpp 
	cwipc_synthetic.cpp 
	cwipc_capturer.cpp 
	cwipc_filters.cpp 
	cwipc_window.cpp 
	cwipc_proxy.cpp 
)

target_sources(cwipc_util
	PRIVATE
	logging.cpp
	"../include/cwipc_util/api.h"
	"../include/cwipc_util/api_pcl.h"
	"../include/cwipc_util/internal/logging.hpp"
	"../include/cwipc_util/internal/capturers.hpp"
)

if(CWIPC_WITH_GUI)
	target_link_libraries(cwipc_util PRIVATE glfw OpenGL::GL OpenGL::GLU)
	target_compile_definitions(cwipc_util PRIVATE CWIPC_WITH_GUI=1)
	target_sources(cwipc_util
		PRIVATE
		window_util.hpp
		window_util.cpp 
	)
endif()

target_include_directories(
    cwipc_util PUBLIC
	$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> 
)
target_include_directories(
    cwipc_util PRIVATE
    ${glfw_INCLUDE_DIRS}
    ${PCL_INCLUDE_DIRS}
)
#if(${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
#	target_link_directories(
#		cwipc_util PUBLIC
#		${PCL_LIBRARY_DIRS}
#	)
#	target_compile_definitions(
#		cwipc_util PRIVATE
#		${PCL_DEFINITIONS}
#	)
#endif()

target_link_libraries(cwipc_util PUBLIC ${PCL_LIBRARIES})
# There is an issue with pcl (or maybe with vcpkg?) that if you build pcl with static libraries
# the library libpcl_io.a will depend on libpcl_io_ply.a but this is not reflected in the definitions.
# So we manually add that library.
if (VCPKG_TARGET_TRIPLET STREQUAL "arm64-android")
	target_link_libraries(cwipc_util PUBLIC ${PCL_IO_PLY_LIBRARY})
endif()
target_link_libraries(cwipc_util PRIVATE nlohmann_json::nlohmann_json)
if(WIN32)
	target_link_libraries(cwipc_util PRIVATE wsock32 ws2_32)
endif()
if(WIN32)
	install(TARGETS cwipc_util
		RUNTIME_DEPENDENCIES 
		PRE_EXCLUDE_REGEXES "api-ms-" "ext-ms-"
		POST_EXCLUDE_REGEXES ".*system32/.*\\.dll"
		EXPORT cwipc_util
		RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
		LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
		ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
		PUBLIC_HEADER DESTINATION include/cwipc_util
	)
else()
	install(TARGETS cwipc_util
	EXPORT cwipc_util
	RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
	LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
	ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
	PUBLIC_HEADER DESTINATION include/cwipc_util
	)
endif()

if(ANDROID)
	file(COPY ${OpenMP_omp_LIBRARY} DESTINATION ${CMAKE_LIBRARY_OUTPUT_DIRECTORY})
	install(FILES ${OpenMP_omp_LIBRARY} DESTINATION ${CMAKE_INSTALL_LIBDIR})
endif()

install(DIRECTORY ${PROJECT_SOURCE_DIR}/include/cwipc_util
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(FILES ${PROJECT_SOURCE_DIR}/CMakeFiles/cwipc_util-config.cmake DESTINATION lib/cmake/cwipc_util)

install(EXPORT cwipc_util DESTINATION lib/cmake/cwipc_util)

if(WIN32)
	# Copy the dependent DLLs that cmake/vcpkg have created
	# install(FILES $<TARGET_RUNTIME_DLLS:cwipc_util> DESTINATION ${CMAKE_INSTALL_BINDIR})
	# Copy the PDB file, if it exists
	install(FILES $<TARGET_PDB_FILE:cwipc_util> DESTINATION ${CMAKE_INSTALL_BINDIR} OPTIONAL)
endif()
