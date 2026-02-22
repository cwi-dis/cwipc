
.. _program_listing_file_cwipc_util_include_cwipc_util_api_pcl.h:

Program Listing for File api_pcl.h
==================================

|exhale_lsh| :ref:`Return to documentation for file <file_cwipc_util_include_cwipc_util_api_pcl.h>` (``cwipc_util/include/cwipc_util/api_pcl.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #ifndef _cwipc_util_api_pcl_h_
   #define _cwipc_util_api_pcl_h_
   
   #ifndef __cplusplus
   #error "api_pcl.h requires C++"
   #endif
   
   #include <pcl/point_cloud.h>
   #include <pcl/point_types.h>
   
   #ifndef _CWIPC_PCL_POINTCLOUD_DEFINED
   #ifndef DOXYGEN_SHOULD_SKIP_THIS
   
   struct EIGEN_ALIGN16 _PointXYZRGBMask
   {
       PCL_ADD_POINT4D; // This adds the members x,y,z which can also be accessed using the point (which is float[4])
       PCL_ADD_RGB;
       EIGEN_MAKE_ALIGNED_OPERATOR_NEW
   };
   
   struct EIGEN_ALIGN16 PointXYZRGBMask : public _PointXYZRGBMask
   {
       inline PointXYZRGBMask (const _PointXYZRGBMask &p)
       {
           x = p.x; y = p.y; z = p.z; data[3] = 1.0f;
           rgba = p.rgba;
       }
       
       inline PointXYZRGBMask ()
       {
           x = y = z = 0.0f;
           data[3] = 1.0f;
           r = g = b = 0;
           a = 0;
       }
       inline PointXYZRGBMask (float _x, float _y, float _z, uint8_t _r, uint8_t _g, uint8_t _b, uint8_t _a)
       {
           x = _x; y = _y; z = _z; data[3] = 1.0f;
           r = _r; g = _g; b = _b; a = _a;
       }
   
       inline PointXYZRGBMask(float _x, float _y, float _z) {
           x = _x;
           y = _y;
           z = _z;
           r = g = b = a = 0;
       }
       friend std::ostream& operator << (std::ostream& os, const PointXYZRGBMask& p);
   };
   
   PCL_EXPORTS std::ostream& operator << (std::ostream& os, const PointXYZRGBMask& p);
   
   POINT_CLOUD_REGISTER_POINT_STRUCT (_PointXYZRGBMask,           // here we assume a XYZ + "test" (as fields)
                                      (float, x, x)
                                      (float, y, y)
                                      (float, z, z)
                                      (std::uint32_t, rgba, rgba)
                                      )
   POINT_CLOUD_REGISTER_POINT_WRAPPER(PointXYZRGBMask, _PointXYZRGBMask)
   #endif // DOXYGEN_SHOULD_SKIP_THIS
   
   typedef PointXYZRGBMask cwipc_pcl_point;
   
   typedef  pcl::shared_ptr<pcl::PointCloud<cwipc_pcl_point>> cwipc_pcl_pointcloud;
   
   inline cwipc_pcl_pointcloud new_cwipc_pcl_pointcloud(void) { return cwipc_pcl_pointcloud(new pcl::PointCloud<cwipc_pcl_point>); }
   #define _CWIPC_PCL_POINTCLOUD_DEFINED
   #endif //_CWIPC_PCL_POINTCLOUD_DEFINED
   
   #ifdef _CWIPC_PCL_POINTCLOUD_PLACEHOLDER_DEFINED
   #error cwipc_pcl_pointcloud placeholder already defined. Did you include api.h before api_pcl.h?
   #endif //_CWIPC_PCL_POINTCLOUD_PLACEHOLDER_DEFINED
   
   #include "cwipc_util/api.h"
    
   _CWIPC_UTIL_EXPORT cwipc_pointcloud *cwipc_from_pcl(cwipc_pcl_pointcloud pc, uint64_t timestamp, char **errorMessage, uint64_t apiVersion);
   
   
   #endif // _cwipc_util_api_pcl_h_
