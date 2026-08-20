
cmake_minimum_required(VERSION 3.16.0)

add_executable(cwipc_check cwipc_check.cpp)
target_compile_definitions(cwipc_check PRIVATE "LIBEXECDIR=\"${CMAKE_INSTALL_FULL_LIBEXECDIR}\"")

install(TARGETS cwipc_check RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# add_test(NAME cwipc_check COMMAND cwipc_util_install_check )
