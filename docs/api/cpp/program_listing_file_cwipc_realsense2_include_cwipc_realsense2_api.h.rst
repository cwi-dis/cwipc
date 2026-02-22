
.. _program_listing_file_cwipc_realsense2_include_cwipc_realsense2_api.h:

Program Listing for File api.h
==============================

|exhale_lsh| :ref:`Return to documentation for file <file_cwipc_realsense2_include_cwipc_realsense2_api.h>` (``cwipc_realsense2/include/cwipc_realsense2/api.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #ifndef cwipc_realsense_api_h
   #define cwipc_realsense_api_h
   
   #include "cwipc_util/api.h"
   
   /* Ensure we have the right dllexport or dllimport on windows */
   #ifndef _CWIPC_REALSENSE2_EXPORT
   #if defined(WIN32) || defined(_WIN32)
   #define _CWIPC_REALSENSE2_EXPORT __declspec(dllimport)
   #else
   #define _CWIPC_REALSENSE2_EXPORT 
   #endif
   #endif
   
   
   #ifdef __cplusplus
   extern "C" {
   #endif
   
   extern _CWIPC_REALSENSE2_EXPORT int CWIPC_RS2_FORMAT_RGB8;  
   extern _CWIPC_REALSENSE2_EXPORT int CWIPC_RS2_FORMAT_Z16;   
   
   _CWIPC_REALSENSE2_EXPORT const char *cwipc_get_version_realsense2();
   
   
   _CWIPC_REALSENSE2_EXPORT cwipc_activesource* cwipc_realsense2(const char *configFilename, char **errorMessage, uint64_t apiVersion);
   
   
   _CWIPC_REALSENSE2_EXPORT cwipc_activesource* cwipc_realsense2_playback(const char *configFilename, char **errorMessage, uint64_t apiVersion);
   #ifdef __cplusplus
   };
   #endif
   
   #endif /* cwipc_realsense_api_h */
