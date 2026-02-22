
.. _program_listing_file_cwipc_util_include_cwipc_util_api.h:

Program Listing for File api.h
==============================

|exhale_lsh| :ref:`Return to documentation for file <file_cwipc_util_include_cwipc_util_api.h>` (``cwipc_util/include/cwipc_util/api.h``)

.. |exhale_lsh| unicode:: U+021B0 .. UPWARDS ARROW WITH TIP LEFTWARDS

.. code-block:: cpp

   #ifndef _cwipc_util_api_h_
   #define _cwipc_util_api_h_
   
   // @file api.h
   // @brief Main header for cwipc API, including both C and C++ interfaces.
   //
   // This header is intended to be the only header needed to use the cwipc API, and it includes both C and C++ interfaces. The C++ interface is more fully featured and is the natural choice when writing native applications, but the C interface is also available for use from other languages (such as Python and C#) and for use in C applications. 
   // The C++ interface is in the form of abstract classes, and the C interface is in the form of opaque pointers to these classes. The C++ interface is more direct and allows for more features, while the C interface is more limited but can be used from other languages and from C applications.
   //
   // 
   #include <stdint.h>
   #include <stdbool.h>
   
   #ifdef __cplusplus
   #include <string>
   #include <set>
   #endif
   
   // For Windows ensure that the symbols are imported from a DLL, unless we're compiling the DLL itself.
   #ifndef _CWIPC_UTIL_EXPORT
   #ifdef WIN32
   #define _CWIPC_UTIL_EXPORT __declspec(dllimport)
   #else
   #define _CWIPC_UTIL_EXPORT
   #endif
   #endif
   
   #define CWIPC_API_VERSION ((uint64_t)0x20260129)
   
   #define CWIPC_API_VERSION_OLD ((uint64_t)0x20260129)
   
   #define CWIPC_CWIPCDUMP_HEADER "cpcd"
   
   #define CWIPC_CWIPCDUMP_VERSION ((uint32_t)0x20210208)
   
   #define CWIPC_FLAG_BINARY 1
   
   
   struct cwipc_cwipcdump_header {
       char hdr[4];    // 0-4
       uint32_t magic; // 4-8
       uint64_t timestamp; // 8-16
       float cellsize; // 16-20
       uint32_t unused; // 20-24
       size_t size; // 24-32
   };
   
   #ifdef __cplusplus
   static_assert(sizeof(struct cwipc_cwipcdump_header) == 32, "cwipc_cwipcdump_header unexpected size");
   #endif
   
   struct cwipc_vector {
       double x;           
       double y;           
       double z;           
   };
   
   struct cwipc_point {
       float x;            
       float y;            
       float z;            
       uint8_t r;          
       uint8_t g;          
       uint8_t b;          
       uint8_t tile;       
   };
   
   struct cwipc_point_packetheader {
       uint32_t magic; 
       uint32_t dataCount; 
       uint64_t timestamp; 
       float cellsize; 
       uint32_t unused;    
   };
   
   #define CWIPC_POINT_PACKETHEADER_MAGIC 0x20201016
   
   struct cwipc_skeleton_joint {
       uint32_t confidence; 
       float x;    
       float y;    
       float z;    
       float q_w;  
       float q_x;  
       float q_y;  
       float q_z;  
   };
   
   struct cwipc_skeleton_collection {
       uint32_t n_skeletons;       
       uint32_t n_joints;  
       struct cwipc_skeleton_joint joints[1];      
   };
   
   struct cwipc_tileinfo {
       struct cwipc_vector normal; 
       char* cameraName;                           
       uint8_t ncamera;                    
       uint8_t cameraMask;         
   };
   
   
   enum cwipc_log_level { CWIPC_LOG_LEVEL_NONE=0, CWIPC_LOG_LEVEL_ERROR=1, CWIPC_LOG_LEVEL_WARNING=2, CWIPC_LOG_LEVEL_TRACE=3, CWIPC_LOG_LEVEL_DEBUG=4 };
   
   typedef void (*cwipc_log_callback_t)(int level, const char* message);
   
   #ifdef __cplusplus
   
   class cwipc_metadata;
   
   #ifndef _CWIPC_PCL_POINTCLOUD_DEFINED
   typedef void* cwipc_pcl_pointcloud;
   #define _CWIPC_PCL_POINTCLOUD_PLACEHOLDER_DEFINED
   #endif //_CWIPC_PCL_POINTCLOUD_DEFINED
   
   class cwipc_pointcloud {
   public: 
       virtual ~cwipc_pointcloud() {};
   
       virtual void free() = 0;
   
       virtual cwipc_pointcloud *_shallowcopy() = 0;
   
       virtual uint64_t timestamp() = 0;
   
       virtual float cellsize() = 0;
   
       virtual void _set_cellsize(float cellsize) = 0;
   
       virtual void _set_timestamp(uint64_t timestamp) = 0;
   
       virtual int count() = 0;
   
   
       virtual size_t get_uncompressed_size() = 0;
   
       virtual int copy_uncompressed(struct cwipc_point* pointbuf, size_t size) = 0;
   
       virtual size_t copy_packet(uint8_t* packet, size_t size) = 0;
   
       virtual cwipc_pcl_pointcloud access_pcl_pointcloud() = 0;
   
       virtual cwipc_metadata* access_metadata() = 0;
   };
   
   class cwipc_source {
   public: 
       virtual ~cwipc_source() {};
   
       virtual void free() = 0;
   
       virtual bool seek(uint64_t timestamp) = 0;
   
       virtual bool eof() = 0;
   
       virtual bool available(bool wait) = 0;
   
       virtual cwipc_pointcloud* get() = 0;
   
   };
   
   class cwipc_activesource : public cwipc_source {
   public:
       virtual ~cwipc_activesource() {};
   
       virtual bool reload_config(const char* configFile) = 0;
   
       virtual size_t get_config(char* buffer, size_t size) = 0;
       virtual bool start() = 0;
   
   
       virtual void stop() = 0;
   
       virtual bool seek(uint64_t timestamp) = 0;
   
       virtual int maxtile() = 0;
   
       virtual bool get_tileinfo(int tilenum, struct cwipc_tileinfo* tileinfo) = 0;
   
   
       virtual void request_metadata(const std::string& name) {
           metadata_wanted.insert(name);
       }
   
       bool is_metadata_requested(const std::string& name) {
           return metadata_wanted.find(name) != metadata_wanted.end();
       }
   
       virtual bool auxiliary_operation(const std::string op, const void* inbuf, size_t insize, void* outbuf, size_t outsize) {
           return false;
       }
   
   private:
       std::set<std::string> metadata_wanted;
   };
   
   class cwipc_sink {
   public: 
       virtual ~cwipc_sink() {};
   
       virtual void free() = 0;
   
       virtual bool feed(cwipc_pointcloud* pc, bool clear) = 0;
   
       virtual bool caption(const char* caption) = 0;
   
       virtual char interact(const char* prompt, const char* responses, int32_t millis) = 0;
   };
   
   class cwipc_metadata {
   public:
       typedef void (*deallocfunc)(void*);
   
       virtual ~cwipc_metadata() {}
   
       virtual int count() = 0;
   
       virtual const std::string& name(int idx) = 0;
   
       virtual const std::string& description(int idx) = 0;
   
       virtual void* pointer(int idx) = 0;
   
       virtual size_t size(int idx) = 0;
   
       virtual void _add(const std::string& name, const std::string& description, void* pointer, size_t size, deallocfunc dealloc) = 0;
   
       virtual void _move(cwipc_metadata* other) = 0;
   };
   
   #else
   
   typedef struct _cwipc_pointcloud {
       int _dummy;
   } cwipc_pointcloud;
   
   typedef struct _cwipc_pcl_pointcloud {
       int _dummy;
   } *cwipc_pcl_pointcloud;
   
   typedef struct _cwipc_source {
       int _dummy;
   } cwipc_source;
   
   typedef struct cwipc_activesource {
       struct _cwipc_source source;
   } cwipc_activesource;
   
   typedef struct _cwipc_sink {
       int _dummy;
   } cwipc_sink;
   
   typedef struct _cwipc_metadata {
       int _dummy;
   } cwipc_metadata;
   
   #endif
   
   #ifdef __cplusplus
   extern "C" {
   #endif
   
       _CWIPC_UTIL_EXPORT const char *cwipc_get_version();
   
       _CWIPC_UTIL_EXPORT void cwipc_log_configure(int level, cwipc_log_callback_t callback);
   
       _CWIPC_UTIL_EXPORT void _cwipc_log_emit(int level, const char* module, const char* message);
   
       _CWIPC_UTIL_EXPORT int cwipc_dangling_allocations(bool log);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_read(const char* filename, uint64_t timestamp, char** errorMessage, uint64_t apiVersion);
   
       _CWIPC_UTIL_EXPORT int cwipc_write(const char* filename, cwipc_pointcloud* pc, char** errorMessage);
   
       _CWIPC_UTIL_EXPORT int cwipc_write_ext(const char* filename, cwipc_pointcloud* pc, int flag, char** errorMessage);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_from_points(struct cwipc_point* points, size_t size, int npoint, uint64_t timestamp, char** errorMessage, uint64_t apiVersion);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_from_packet(uint8_t* packet, size_t size, char** errorMessage, uint64_t apiVersion);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_read_debugdump(const char* filename, char** errorMessage, uint64_t apiVersion);
   
       _CWIPC_UTIL_EXPORT int cwipc_write_debugdump(const char* filename, cwipc_pointcloud* pc, char** errorMessage);
   
   
       _CWIPC_UTIL_EXPORT void cwipc_pointcloud_free(cwipc_pointcloud* pc);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud *cwipc_pointcloud__shallowcopy(cwipc_pointcloud* pc);
       
       _CWIPC_UTIL_EXPORT uint64_t cwipc_pointcloud_timestamp(cwipc_pointcloud* pc);
   
       _CWIPC_UTIL_EXPORT float cwipc_pointcloud_cellsize(cwipc_pointcloud* pc);
   
       _CWIPC_UTIL_EXPORT void cwipc_pointcloud__set_cellsize(cwipc_pointcloud* pc, float cellsize);
   
       _CWIPC_UTIL_EXPORT void cwipc_pointcloud__set_timestamp(cwipc_pointcloud* pc, uint64_t timestamp);
   
       _CWIPC_UTIL_EXPORT int cwipc_pointcloud_count(cwipc_pointcloud* pc);
   
       _CWIPC_UTIL_EXPORT size_t cwipc_pointcloud_get_uncompressed_size(cwipc_pointcloud* pc);
   
       _CWIPC_UTIL_EXPORT int cwipc_pointcloud_copy_uncompressed(cwipc_pointcloud* pc, struct cwipc_point* pointbuf, size_t size);
   
       _CWIPC_UTIL_EXPORT size_t cwipc_pointcloud_copy_packet(cwipc_pointcloud* pc, uint8_t* packet, size_t size);
   
       _CWIPC_UTIL_EXPORT cwipc_metadata* cwipc_pointcloud_access_metadata(cwipc_pointcloud* pc);
   
       _CWIPC_UTIL_EXPORT bool cwipc_activesource_start(cwipc_activesource* src);
   
       _CWIPC_UTIL_EXPORT void cwipc_activesource_stop(cwipc_activesource* src);
       
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_source_get(cwipc_source* src);
   
       _CWIPC_UTIL_EXPORT void cwipc_source_free(cwipc_source* src);
   
       _CWIPC_UTIL_EXPORT bool cwipc_source_eof(cwipc_source* src);
   
       _CWIPC_UTIL_EXPORT bool cwipc_source_available(cwipc_source* src, bool wait);
   
       _CWIPC_UTIL_EXPORT void cwipc_activesource_request_metadata(cwipc_activesource* src, const char* name);
   
       _CWIPC_UTIL_EXPORT bool cwipc_activesource_is_metadata_requested(cwipc_activesource* src, const char* name);
   
       _CWIPC_UTIL_EXPORT bool cwipc_activesource_reload_config(cwipc_activesource* src, const char* configFile);
   
       _CWIPC_UTIL_EXPORT size_t cwipc_activesource_get_config(cwipc_activesource* src, char* buffer, size_t size);
   
       _CWIPC_UTIL_EXPORT bool cwipc_activesource_seek(cwipc_activesource* src, uint64_t timestamp);
   
       _CWIPC_UTIL_EXPORT int cwipc_activesource_maxtile(cwipc_activesource* src);
   
       _CWIPC_UTIL_EXPORT bool cwipc_activesource_get_tileinfo(cwipc_activesource* src, int tilenum, struct cwipc_tileinfo* tileinfo);
   
       _CWIPC_UTIL_EXPORT bool cwipc_activesource_auxiliary_operation(cwipc_activesource *src, const char* op, const void* inbuf, size_t insize, void* outbuf, size_t outsize);
   
   
       _CWIPC_UTIL_EXPORT void cwipc_sink_free(cwipc_sink* sink);
   
       _CWIPC_UTIL_EXPORT bool cwipc_sink_feed(cwipc_sink* sink, cwipc_pointcloud* pc, bool clear);
   
       _CWIPC_UTIL_EXPORT bool cwipc_sink_caption(cwipc_sink* sink, const char* caption);
   
       _CWIPC_UTIL_EXPORT char cwipc_sink_interact(cwipc_sink* sink, const char* prompt, const char* responses, int32_t millis);
   
       _CWIPC_UTIL_EXPORT void cwipc_metadata__move(cwipc_metadata*  src, cwipc_metadata* dest);
       
       _CWIPC_UTIL_EXPORT int cwipc_metadata_count(cwipc_metadata* collection);
   
       _CWIPC_UTIL_EXPORT const char* cwipc_metadata_name(cwipc_metadata* collection, int idx);
   
       _CWIPC_UTIL_EXPORT const char* cwipc_metadata_description(cwipc_metadata* collection, int idx);
   
   
       _CWIPC_UTIL_EXPORT void* cwipc_metadata_pointer(cwipc_metadata* collection, int idx);
   
       _CWIPC_UTIL_EXPORT size_t cwipc_metadata_size(cwipc_metadata* collection, int idx);
   
       _CWIPC_UTIL_EXPORT cwipc_activesource* cwipc_synthetic(int fps, int npoints, char** errorMessage, uint64_t apiVersion);
   
       _CWIPC_UTIL_EXPORT cwipc_activesource* cwipc_capturer(const char *configFilename, char** errorMessage, uint64_t apiVersion);
   
       _CWIPC_UTIL_EXPORT cwipc_sink* cwipc_window(const char* title, char** errorMessage, uint64_t apiVersion);
   
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_downsample(cwipc_pointcloud* pc, float voxelsize);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_remove_outliers(cwipc_pointcloud* pc, int kNeighbors, float stddevMulThresh, bool perTile);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_tilefilter(cwipc_pointcloud* pc, int tile);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_tilemap(cwipc_pointcloud* pc, uint8_t map[256]);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_crop(cwipc_pointcloud* pc, float bbox[6]);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_colormap(cwipc_pointcloud* pc, uint32_t clearBits, uint32_t setBits);
   
       _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_join(cwipc_pointcloud* pc1, cwipc_pointcloud* pc2);
   
       _CWIPC_UTIL_EXPORT cwipc_activesource* cwipc_proxy(const char* host, int port, char** errorMessage, uint64_t apiVersion);
   
   #ifdef __cplusplus
   }
   #endif
   
   #endif // _cwipc_util_api_h_
