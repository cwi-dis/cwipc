
.. _program_listing_file_cwipc_orbbec_include_cwipc_orbbec_api.h:

Program Listing for File api.h
==============================

|exhale_lsh| :ref:`Return to documentation for file <file_cwipc_orbbec_include_cwipc_orbbec_api.h>` (``cwipc_orbbec/include/cwipc_orbbec/api.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #ifndef cwipc_orbbec_api_h
   #define cwipc_orbbec_api_h
   
   #include "cwipc_util/api.h"
   
   /* Ensure we have the right dllexport or dllimport on windows */
   #ifndef _CWIPC_ORBBEC_EXPORT
   #if defined(WIN32) || defined(_WIN32)
   #define _CWIPC_ORBBEC_EXPORT __declspec(dllimport)
   #else
   #define _CWIPC_ORBBEC_EXPORT 
   #endif
   #endif
   
   #ifdef __cplusplus
   extern "C" {
   #endif
   _CWIPC_ORBBEC_EXPORT const char *cwipc_get_version_orbbec();
   
   
   _CWIPC_ORBBEC_EXPORT cwipc_activesource* cwipc_orbbec(const char *configFilename, char **errorMessage, uint64_t apiVersion);
   
   
   
   
   _CWIPC_ORBBEC_EXPORT cwipc_activesource* cwipc_orbbec_playback(const char* configFilename, char** errorMessage, uint64_t apiVersion);
   
   
   #ifdef __cplusplus
   }
   #endif
   
   
   #endif /* cwipc_orbbec_api_h */
