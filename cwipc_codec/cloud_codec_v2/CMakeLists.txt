
cmake_minimum_required(VERSION 3.16.0)

set(SUBSYS_NAME cloud_codec_v2)
set(SUBSYS_DESC "Extensions for Point Cloud Compression for JPEG I/O")

set(CMAKE_CXX_STANDARD 14)

#if(EXISTS "${PCL_ROOT}/include/pcl-${PCL_VERSION_MAJOR}.${PCL_VERSION_MINOR}/pcl/io/impl/entropy_range_coder.hpp")
#    set(PCL_INSTALLED ON CACHE INTERNAL "PCL Installed" FORCE)
#endif()

#set(${PROJECT_NAME}_INCLUDE_DIRS
#  ${CMAKE_CURRENT_SOURCE_DIR}/include
#  ${${PROJECT_NAME}_INCLUDE_DIRS}
#  CACHE INTERNAL "${PROJECT_NAME}: Include Directories" FORCE)
#if(CMAKELISTS_DEBUG)
#  message("${SUBSYS_NAME}-${PROJECT_NAME}_INCLUDE_DIRS: " ${${PROJECT_NAME}_INCLUDE_DIRS})
#endif(CMAKELISTS_DEBUG)

set(srcs
  "src/point_cloud_codec_v2.cpp"
  "src/jpeg_io.cpp"
)
set(LIB_NAME ${SUBSYS_NAME})
set(incs
  "include/pcl/${SUBSYS_NAME}/point_cloud_codec_v2.h"
  "include/pcl/${SUBSYS_NAME}/point_cloud_codec_v2_omp.h"
  "include/pcl/${SUBSYS_NAME}/point_coding_v2.h"
  "include/pcl/${SUBSYS_NAME}/color_coding_jpeg.h"
  "include/pcl/${SUBSYS_NAME}/jpeg_io.h"
  "include/pcl/${SUBSYS_NAME}/snake_grid_mapping.h"
  "include/pcl/${SUBSYS_NAME}/color_coding_jpeg.h"
  "include/pcl/${SUBSYS_NAME}/quaternion_coding.h"
  "include/pcl/${SUBSYS_NAME}/rigid_transform_coding.h"
 )
set(impl_incs
   "include/pcl/${SUBSYS_NAME}/impl/point_cloud_codec_v2_impl.hpp"
   "include/pcl/${SUBSYS_NAME}/impl/point_cloud_codec_v2_omp_impl.hpp"
   "include/pcl/${SUBSYS_NAME}/impl/quaternion_coding_impl.hpp"
   "include/pcl/${SUBSYS_NAME}/impl/rigid_transform_coding_impl.hpp"
   )
#if(${PCL_INSTALLED})
#    add_definitions(-DPCL_INSTALLED)
#endif(${PCL_INSTALLED})

#include_directories(${${PROJECT_NAME}_INCLUDE_DIRS})
add_library(${LIB_NAME} STATIC ${srcs} ${incs} ${impl_incs})
set_property(TARGET ${LIB_NAME} PROPERTY POSITION_INDEPENDENT_CODE ON)
#target_link_libraries(${LIB_NAME} ${${PROJECT_NAME}_LIBRARIES})

target_include_directories(${LIB_NAME}
    PRIVATE
    include
)

target_include_directories(${LIB_NAME}
    PUBLIC
    $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include> 
    ${PCL_INCLUDE_DIRS}
)

target_compile_definitions(${LIB_NAME} PRIVATE -DPCL_INSTALLED ${PCL_DEFINITIONS})
target_link_libraries(${LIB_NAME} PUBLIC ${PCL_LIBRARIES} $<IF:$<TARGET_EXISTS:libjpeg-turbo::jpeg>,libjpeg-turbo::jpeg,libjpeg-turbo::jpeg-static>)
#set(${PROJECT_NAME}_LIBRARY_DIRS
#  ${CMAKE_CURRENT_BINARY_DIR}
#  ${${PROJECT_NAME}_LIBRARY_DIRS}
#  CACHE INTERNAL "${PROJECT_NAME}: Library Directories" FORCE)
#if(CMAKELISTS_DEBUG)
#  message("${SUBSYS_NAME}-${PROJECT_NAME}_LIBRARY_DIRS: " ${${PROJECT_NAME}_LIBRARY_DIRS})
#endif(CMAKELISTS_DEBUG)

#set(${PROJECT_NAME}_LIBRARIES
#  ${LIB_NAME}
#  ${${PROJECT_NAME}_LIBRARIES}
#  CACHE INTERNAL "${PROJECT_NAME}:Libraries" FORCE)
#if(CMAKELISTS_DEBUG)
#  message("${SUBSYS_NAME}-${PROJECT_NAME}_LIBRARIES: " ${${PROJECT_NAME}_LIBRARIES})
#endif(CMAKELISTS_DEBUG)
