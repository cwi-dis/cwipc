cmake_minimum_required(VERSION 3.16.0)

file(COPY fixtures/input/pcl_frame1.ply DESTINATION ${CMAKE_TESTDATA_OUTPUT_DIRECTORY}/fixtures/input/)
install(FILES fixtures/input/pcl_frame1.ply DESTINATION ${CMAKE_TESTDATA_INSTALL_DIRECTORY}/fixtures/input/)
message(STATUS "xxxjack copied fixtures to ${CMAKE_TESTDATA_OUTPUT_DIRECTORY}")