cmake_minimum_required(VERSION 3.28.0)

add_test(NAME python_compressfile 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/compressfile.py ${CMAKE_SOURCE_DIR}/cwipc_codec/tests/fixtures/input/pcl_frame1.ply ./pcl_frame1.cwicpc
)
cwipc_python_test(TEST python_compressfile FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME python_decompressfile
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/decompressfile.py ${CMAKE_SOURCE_DIR}/cwipc_codec/tests/fixtures/compressed/pcl_frame1.cwicpc ./pcl_frame1.ply
)
cwipc_python_test(TEST python_decompressfile FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME python_downsample
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/downsample.py  0.1  ${CMAKE_SOURCE_DIR}/cwipc_util/tests/fixtures/input/pcl_frame1.ply ./pcl_frame1_downsampled.ply 
)
cwipc_python_test(TEST python_downsample FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME python_dump2ply 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/dump2ply.py ${CMAKE_SOURCE_DIR}/cwipc_util/tests/fixtures/input/pcl_frame1.cwipcdump ./pcl_frame1.ply 
)
cwipc_python_test(TEST python_dump2ply FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME python_generate 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/generate.py 2 .
)
cwipc_python_test(TEST python_generate FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME python_ply2dump 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/ply2dump.py ${CMAKE_SOURCE_DIR}/cwipc_util/tests/fixtures/input/pcl_frame1.ply ./pcl_frame1.cwipcdump
)
cwipc_python_test(TEST python_ply2dump FIXTURES_REQUIRED venv_with_cwipc)

add_test(NAME python_record 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/record.py 2 .
)
cwipc_python_test(TEST python_record FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(python_record PROPERTIES DISABLED TRUE)

add_test(NAME python_viewcamera 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/viewcamera.py
)
cwipc_python_test(TEST python_viewcamera FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(python_viewcamera PROPERTIES DISABLED TRUE)

add_test(NAME python_viewfile 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/viewfile.py ${CMAKE_SOURCE_DIR}/cwipc_util/tests/fixtures/input/pcl_frame1.ply
)
cwipc_python_test(TEST python_viewfile FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(python_viewfile PROPERTIES DISABLED TRUE)

add_test(NAME python_viewrecording 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/viewrecording.py
)
cwipc_python_test(TEST python_viewrecording FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(python_viewrecording PROPERTIES DISABLED TRUE)

add_test(NAME python_viewsynthetic 
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/viewsynthetic.py
)
cwipc_python_test(TEST python_viewsynthetic FIXTURES_REQUIRED venv_with_cwipc)
set_tests_properties(python_viewsynthetic PROPERTIES DISABLED TRUE)
