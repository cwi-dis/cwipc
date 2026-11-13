
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_check cwipc_check.cpp)
# std::filesystem. This target links nothing that requires C++17 (cwi-dis/cwipc#361).
target_compile_features(cwipc_check PRIVATE cxx_std_17)
# Fallback only: cwipc_check finds its tree from its own location (cwi-dis/cwipc#355).
target_compile_definitions(cwipc_check PRIVATE "LIBEXECDIR=\"${CMAKE_INSTALL_FULL_LIBEXECDIR}\"")
if(WIN32)
	target_link_libraries(cwipc_check PRIVATE shell32)	# IsUserAnAdmin
endif()

install(TARGETS cwipc_check RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# add_test(NAME cwipc_check COMMAND cwipc_util_install_check )
