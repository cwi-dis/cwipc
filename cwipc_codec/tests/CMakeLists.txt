cmake_minimum_required(VERSION 3.16.0)

file(COPY fixtures/input/pcl_frame1.ply fixtures/input/loot_tiled.ply DESTINATION ${CMAKE_TESTDATA_OUTPUT_DIRECTORY}/fixtures/input/)
file(COPY fixtures/compressed DESTINATION ${CMAKE_TESTDATA_OUTPUT_DIRECTORY}/fixtures/ )
install(FILES fixtures/input/pcl_frame1.ply fixtures/input/loot_tiled.ply DESTINATION ${CMAKE_TESTDATA_INSTALL_DIRECTORY}/fixtures/input/)
install(DIRECTORY fixtures/compressed DESTINATION ${CMAKE_TESTDATA_INSTALL_DIRECTORY}/fixtures/)
