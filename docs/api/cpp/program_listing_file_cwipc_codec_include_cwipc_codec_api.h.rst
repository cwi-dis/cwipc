
.. _program_listing_file_cwipc_codec_include_cwipc_codec_api.h:

Program Listing for File api.h
==============================

|exhale_lsh| :ref:`Return to documentation for file <file_cwipc_codec_include_cwipc_codec_api.h>` (``cwipc_codec/include/cwipc_codec/api.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #ifndef cwipc_codec_api_h
   #define cwipc_codec_api_h
   #include <stdint.h>
   #include "cwipc_util/api.h"
   #ifdef __cplusplus
   #include <sstream>
   #endif
   
   // For Windows ensure that the symbols are imported from a DLL, unless we're compiling the DLL itself.
   #ifndef _CWIPC_CODEC_EXPORT
   #ifdef WIN32
   #define _CWIPC_CODEC_EXPORT __declspec(dllimport)
   #else
   #define _CWIPC_CODEC_EXPORT
   #endif
   #endif
   
   struct cwipc_encoder_params
   {
       bool do_inter_frame;        
       int gop_size;                       
       float exp_factor;           
       int octree_bits;            
       int jpeg_quality;           
       int macroblock_size;        
       int tilenumber;                     
       float voxelsize;            
       int n_parallel;             
   };
   
   #define CWIPC_ENCODER_PARAM_VERSION ((uint32_t)0x20220607)
   #define CWIPC_ENCODER_PARAM_VERSION_OLD ((uint32_t)0x20190506)
   
   #ifdef __cplusplus
   
   class cwipc_encoder
   {
   public: 
       virtual ~cwipc_encoder() {}
   
       virtual void free() = 0;
       
       virtual void feed(cwipc_pointcloud *pc) = 0;
   
       virtual void close() = 0;
       
       virtual bool eof() = 0;
   
       virtual bool available(bool wait) = 0;
   
       virtual size_t get_encoded_size() = 0;
       
       virtual bool copy_data(void *buffer, size_t bufferSize) = 0;
       
        virtual bool at_gop_boundary() = 0;
   };
   
   class cwipc_encodergroup
   {
   public:
       virtual ~cwipc_encodergroup() {}
   
       virtual void free() = 0;
   
       virtual void feed(cwipc_pointcloud *pc) = 0;
       
       virtual void close() = 0;
   
       virtual cwipc_encoder *addencoder(int version, cwipc_encoder_params* params, char **errorMessage) = 0;
   };
   
   class cwipc_decoder : public cwipc_source
   {
   public:
       virtual ~cwipc_decoder() {}
       virtual void free() = 0;
       virtual bool eof() = 0;
       virtual bool available(bool wait) = 0;
       virtual cwipc_pointcloud *get() = 0;
   
       virtual void feed(void *buffer, size_t bufferSize) = 0;
   
       virtual void close() = 0;
   };
   #else
   typedef struct _cwipc_encoder {
       int _dummy;
   } cwipc_encoder;
   
   typedef struct _cwipc_encodergroup {
       int _dummy;
   } cwipc_encodergroup;
   
   typedef struct _cwipc_decoder {
       cwipc_source source;
   } cwipc_decoder;
   #endif
   
   #ifdef __cplusplus
   extern "C" {
   #endif
   _CWIPC_CODEC_EXPORT const char *cwipc_get_version_codec();
   
   _CWIPC_CODEC_EXPORT cwipc_encoder* cwipc_new_encoder(int version, cwipc_encoder_params* params, char **errorMessage, uint64_t apiVersion);
   
   _CWIPC_CODEC_EXPORT void cwipc_encoder_free(cwipc_encoder *obj);
   
   _CWIPC_CODEC_EXPORT bool cwipc_encoder_eof(cwipc_encoder *obj);
   
   _CWIPC_CODEC_EXPORT bool cwipc_encoder_available(cwipc_encoder *obj, bool wait);
   
   _CWIPC_CODEC_EXPORT void cwipc_encoder_feed(cwipc_encoder *obj, cwipc_pointcloud* pc);
   
   _CWIPC_CODEC_EXPORT void cwipc_encoder_close(cwipc_encoder *obj);
   
   _CWIPC_CODEC_EXPORT size_t cwipc_encoder_get_encoded_size(cwipc_encoder *obj);
   
   _CWIPC_CODEC_EXPORT bool cwipc_encoder_copy_data(cwipc_encoder *obj, void *buffer, size_t bufferSize);
   
    _CWIPC_CODEC_EXPORT bool cwipc_encoder_at_gop_boundary(cwipc_encoder *obj);
   
   _CWIPC_CODEC_EXPORT cwipc_encodergroup *cwipc_new_encodergroup(char **errorMessage, uint64_t apiVersion);
   
   _CWIPC_CODEC_EXPORT void cwipc_encodergroup_free(cwipc_encodergroup *obj);
   
   
   _CWIPC_CODEC_EXPORT cwipc_encoder *cwipc_encodergroup_addencoder(cwipc_encodergroup *obj, int version, cwipc_encoder_params* params, char **errorMessage);
   
   _CWIPC_CODEC_EXPORT void cwipc_encodergroup_feed(cwipc_encodergroup *obj, cwipc_pointcloud* pc);
   
   _CWIPC_CODEC_EXPORT void cwipc_encodergroup_close(cwipc_encodergroup *obj);
   
   
   _CWIPC_CODEC_EXPORT cwipc_decoder* cwipc_new_decoder(char **errorMessage, uint64_t apiVersion);
   
   /* Methods defined in the cwipc_source superclass, use those */
   #define cwipc_decoder_free(obj) cwipc_source_free(obj)
   #define cwipc_decoder_available(obj, wait) cwipc_source_available(obj, wait)
   #define cwipc_decoder_eof(obj) cwipc_source_eof(obj)
   #define cwipc_decoder_get(obj) cwipc_source_get(obj)
   
   _CWIPC_CODEC_EXPORT void cwipc_decoder_feed(cwipc_decoder *obj, void *buffer, size_t bufferSize);
   
   _CWIPC_CODEC_EXPORT void cwipc_decoder_close(cwipc_decoder *obj);
   
   
   #ifdef __cplusplus
   }
   #endif
   #endif /* cwipc_codec_api_h */
