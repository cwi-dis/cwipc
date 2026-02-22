
.. _program_listing_file_cwipc_kinect_include_cwipc_kinect_api.h:

Program Listing for File api.h
==============================

|exhale_lsh| :ref:`Return to documentation for file <file_cwipc_kinect_include_cwipc_kinect_api.h>` (``cwipc_kinect/include/cwipc_kinect/api.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #ifndef cwipc_kinect_api_h
   #define cwipc_kinect_api_h
   
   #include "cwipc_util/api.h"
   
   /* Ensure we have the right dllexport or dllimport on windows */
   #ifndef _CWIPC_KINECT_EXPORT
   #if defined(WIN32) || defined(_WIN32)
   #define _CWIPC_KINECT_EXPORT __declspec(dllimport)
   #else
   #define _CWIPC_KINECT_EXPORT 
   #endif
   #endif
   
   #ifdef __cplusplus
   extern "C" {
   #endif
   
   
   _CWIPC_KINECT_EXPORT const char *cwipc_get_version_kinect();
   
   
   _CWIPC_KINECT_EXPORT cwipc_activesource* cwipc_kinect(const char *configFilename, char **errorMessage, uint64_t apiVersion);
   
   
   
   
   _CWIPC_KINECT_EXPORT cwipc_activesource* cwipc_kinect_playback(const char* configFilename, char** errorMessage, uint64_t apiVersion);
   
   #ifdef __cplusplus
   }
   #endif
   
   
   #endif /* cwipc_kinect_api_h */
