#ifndef _cwipc_util_api_h_
#define _cwipc_util_api_h_

/** \file cwipc_util/api.h
 * Main header for cwipc API, including both C and C++ interfaces.
 *
 * This header is intended to be the only header needed to use the cwipc API, and it includes both C and C++ interfaces. The C++ interface is more fully featured and is the natural choice when writing native applications, but the C interface is also available for use from other languages (such as Python and C#) and for use in C applications. 
 * The C++ interface is in the form of abstract classes, and the C interface is in the form of opaque pointers to these classes. The C++ interface is more direct and allows for more features, while the C interface is more limited but can be used from other languages and from C applications.
 */

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

/** \brief Version of cwipc API.
*
* Version of the current API of cwipc. Pass to constructors to ensure library
* compatibility.
*/
#define CWIPC_API_VERSION ((uint64_t)0x20260129)

/** \brief Version of oldest compatible cwipc API.
*
* Version of the oldest API of cwipc to which this set of libraries is compatible.
*/
#define CWIPC_API_VERSION_OLD ((uint64_t)0x20260129)

/** \brief 4 characters that are magic number of cwipcdump file format
*/
#define CWIPC_CWIPCDUMP_HEADER "cpcd"

/** \brief Magic number (version) of cwipcdump file format
*/
#define CWIPC_CWIPCDUMP_VERSION ((uint32_t)0x20210208)

/** \brief flags for cwipc_write_ext. 1=write binary ply file
    */
#define CWIPC_FLAG_BINARY 1

     /** \brief Header of cwipcdump file
      *
      * The header is followed by `size` bytes of data, which should be an integral number of
      * of `cwipc_point` structures.
      */

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

/** \brief Single 3D vector.
 *
 * Coordinates of a single 3D point.
 *
 */
struct cwipc_vector {
    double x;		/**< coordinate */
    double y;		/**< coordinate */
    double z;		/**< coordinate */
};

/** \brief Single point structure.
 *
 * All data pertaining to a single point (in the external representation).
 *
 */
struct cwipc_point {
    float x;		/**< coordinate */
    float y;		/**< coordinate */
    float z;		/**< coordinate */
    uint8_t r;		/**< color */
    uint8_t g;		/**< color */
    uint8_t b;		/**< color */
    uint8_t tile;	/**< tile number (can also be interpreted as mask of contributing cameras) */
};

/** \brief header for transferring cwipc_point data over a network connection
 */
struct cwipc_point_packetheader {
    uint32_t magic; /**< magic number */
    uint32_t dataCount; /**< Number of bytes following this header */
    uint64_t timestamp; /**< Timestamp of the pointcloud */
    float cellsize; /**< Size of a single point */
    uint32_t unused;    /**< extra field to fill out structure to 24 bytes (multiple of 8) */
};

/** \brief magic number for use in cwipc_point_packetheader
 */
#define CWIPC_POINT_PACKETHEADER_MAGIC 0x20201016

 /** \brief Per-joint skeleton information.
  *
  * x, y, z are the coordinates in 3D space of this joint.
  * q_w, q_x, q_y, q_z are the quaternion of the joint orientation.
  * confidence is the value reported by the k4abt module.
  */
struct cwipc_skeleton_joint {
    uint32_t confidence; /**< Confidence value as reported by k4abt library */
    float x;	/**< coordinate */
    float y;	/**< coordinate */
    float z;	/**< coordinate */
    float q_w;	/**< quaternion value */
    float q_x;	/**< quaternion value */
    float q_y;	/**< quaternion value */
    float q_z;	/**< quaternion value */
};

/** \brief All skeleton information returned by k4abt body tracker.
 *
 * n_skeletons is the total number of skeletons found (by all cameras),
 * n_joints is the number of joints per skeleton.
 * joints contains all joints in order.
 *
 * See k4abt documentation for the order of the joints in each skeleton.
 */
struct cwipc_skeleton_collection {
    uint32_t n_skeletons;	/**< Number of skeletons in this collection */
    uint32_t n_joints;	/**< Number of joints per skeleton */
    struct cwipc_skeleton_joint joints[1];	/**< joint values */
};

 /** \brief Information on a tile.
  *
  * Information on tiles with a certain tile number, a vector of length 1 indicating which way the tile is pointing.
  * Tiles that face in no particular direction have length 0.
  * The structure also has information on how many cameras contribute to this tile, and for single-camera
  * tiles a pointer to a unique ID of the camera (static string).
  */
struct cwipc_tileinfo {
    struct cwipc_vector normal;	/**< Vector indicating the direction the tile is facing, often camera position */
    char* cameraName; 				/**< Identifier of the camera (static string) or NULL */
    uint8_t ncamera; 			/**< Number of physical cameras that contribute to this tile */
    uint8_t cameraMask;         /**< Bit mask for this tile */
};


/** \brief Log levels */
enum cwipc_log_level { CWIPC_LOG_LEVEL_NONE=0, CWIPC_LOG_LEVEL_ERROR=1, CWIPC_LOG_LEVEL_WARNING=2, CWIPC_LOG_LEVEL_TRACE=3, CWIPC_LOG_LEVEL_DEBUG=4 };

/** \brief Callback function signature for capturing log output */
typedef void (*cwipc_log_callback_t)(int level, const char* message);

#ifdef __cplusplus

class cwipc_metadata;

#ifndef DOXYGEN_SHOULD_SKIP_THIS
#ifndef _CWIPC_PCL_POINTCLOUD_DEFINED
typedef void* cwipc_pcl_pointcloud;
#define _CWIPC_PCL_POINTCLOUD_PLACEHOLDER_DEFINED
#endif //_CWIPC_PCL_POINTCLOUD_DEFINED
#endif // DOXYGEN_SHOULD_SKIP_THIS

/** \brief Abstract interface to a single pointcloud.
 *
 * This structure wraps a single pointcloud, and is suitable for passing around
 * without caring about the details of point data (and without having to include
 * all of the PCL headers and such). The object can be passed between different
 * shared libraries, different languages, etc.
 *
 * When used from C (as opposed to C++) this is an opaque struct _cwipc pointer.
 */
class cwipc_pointcloud {
public:
    /** \brief Destructor. Does not free underlying pointcloud data.
     * 
     * You must call free() before calling the destructor (unless you have passed
     * the cwipc object across an implementation language boundary and a reference
     * may still be held there).
     */ 
    virtual ~cwipc_pointcloud() {};

    /** \brief Deallocate the pointcloud data.
     *
     * The internal implementation of the pointcloud data may be refcounted, but
     * an explicit free method must be called to ensure the refcounting does not
     * inadvertantly free the pointcloud when dereferenced from a different DLL or
     * implementation language.
     *
     * Whoever created the cwipc object in the first place is responsible for
     * calling free, and the correct DLL will be invoked to actually free the data.
     */
    virtual void free() = 0;

    /** \brief Create a shallow copy of this point cloud
     * For internal use only: returns a new cwipc_pointcloud that shares the underlying (refcounted) data.
     */
    virtual cwipc_pointcloud *_shallowcopy() = 0;

    /** \brief Time this pointcloud was captured.
     * \return Time in milliseconds, since some unspecified origin.
     */
    virtual uint64_t timestamp() = 0;

    /** \brief Returns the grid cell size at which this pointcloud was created, if known.
     * \return Either a size (in the same coordinates as x, y and z) or 0 if unknown.
     *
     * If the pointcloud is to be displayed the number returned by this call is a good
     * guess for a pointsize to use to show an obect that does not have any holes in it.
     */
    virtual float cellsize() = 0;

    /** \brief Semi-private method to initialize the cellsize. Not for general use.
     * Passing a negative number will use a heuristic to determine a reasonable value for
     * the cellsize.
     */
    virtual void _set_cellsize(float cellsize) = 0;

    /** \brief Semi-private method to initialize the timestamp. Not for general use.
     */
    virtual void _set_timestamp(uint64_t timestamp) = 0;

    /** \brief Returns the number of points in the pointcloud.
     *  \return the point count
     */
    virtual int count() = 0;

    /** \brief Returns size (in bytes) an external representation of this pointcloud needs.
     * \return The number of bytes needed (or zero in case the format does not match
     *   or no points are available).
     */

    virtual size_t get_uncompressed_size() = 0;

    /** \brief Get points from pointcloud in external representation format.
     * \param pointbuf A databuffer pointer.
     * \param size The size of the databuffer (in bytes).
     * \return The number of points.
     *
     * The caller is responsible for first calling get_uncompressed_size() and
     * then allocating the data. This is to ensure that the memory is allocated in
     * a way that is compatible with the caller (for example if the caller is
     * implemented in another language like C# or Python and needs a special
     * allocator).
     */
    virtual int copy_uncompressed(struct cwipc_point* pointbuf, size_t size) = 0;

    /** \brief Get pointcloud in external representation format.
     * \param packet A databuffer pointer.
     * \param size The size of the databuffer (in bytes).
     * \return The size of the databuffer
     *
     * Call with packet=NULL to obtain packet buffer size. Then allocate a buffer and
     * call with buffer and size to copy the packet data.
     */
    virtual size_t copy_packet(uint8_t* packet, size_t size) = 0;

    /** \brief Access PCL pointcloud.
     * \return A reference to the PCL pointcloud.
     *
     * Note that this function returns a borrowed reference, it is not guaranteed
     * that the reference remains valid after free() is called.
     */
    virtual cwipc_pcl_pointcloud access_pcl_pointcloud() = 0;

    /** \brief Access metadata collection.
     * \return A reference to the metadata collection.
     *
     * Note that this function returns a borrowed reference (and that the collection consists of more
     * borrowed references). These references become invalid when free() is called.
     */
    virtual cwipc_metadata* access_metadata() = 0;
};

/** \brief A generator of pointclouds, abstract C++ interface.
 *
 * This interface is provided by capturers and decoders and such. It allows the
 * user of this interface to get cwipc pointcloud data.
 */
class cwipc_source {
public:
    /** \brief Destructor. Does not free underlying generator.
     * 
     * You must call free() before calling the destructor (unless you have passed
     * the cwipc object across an implementation language boundary and a reference
     * may still be held there).
     */ 
    virtual ~cwipc_source() {};

    /** \brief Deallocate the pointcloud source.
     *
     * Because the pointcloud source may be used in a different implementation
     * language or DLL than where it is implemented we do not count on refcounting
     * and such. Call this method if you no longer need the source.
     */
    virtual void free() = 0;

    /** \brief Position the source to a specific timestamp.
     * 
     * \param timestamp The timestamp to seek to.
     * \return True if the seek was successful.
     */
    virtual bool seek(uint64_t timestamp) = 0;

    /** \brief Return true if no more pointclouds are forthcoming.
     */
    virtual bool eof() = 0;

    /** \brief Return true if a pointcloud is currently available.
     * \param wait Set to true if the caller is willing to wait until a pointcloud is available.
     *
     * If this cwipc_source is not multi-threading capable the wait parameter is ignored.
     * If it is multi-threaded aware and no pointcloud is currently available
     * it may wait a reasonable amount of time (think: about a second) to see whether
     * one becomes available.
     */
    virtual bool available(bool wait) = 0;

    /** \brief Get a new pointcloud.
     * \return The new pointcloud.
     */
    virtual cwipc_pointcloud* get() = 0;

};

/** \brief A generator of pointclouds based on some capturer, abstract C++ interface.
 *
 * This interface is provided by capturers and such that can
 * return pointclouds. It is a subclass of
 * cwipc_source with extra methods to obtain information on available
 * tiles in the produced pointclouds, plus extensions for reloading the capturer,
 * or positioning it, starting it, stopping it, etc.
 */
class cwipc_activesource : public cwipc_source {
public:
    virtual ~cwipc_activesource() {};

    /** \brief Reload capturer based on a new configuration
    * 
    * Only implemented for actual cameras (realsense, kinect). Closes the cameras and reopens them with a new
    * configuration.
    * 
    * \param configFile The pathname to the new camera configuration or an inline json configuration string.
    * \return A boolean that is true if the reload was successful.
    */
    virtual bool reload_config(const char* configFile) = 0;

    /** \brief Reload parts of the capturer based on a new configuration
    *
    * Only implemented for actual cameras (realsense, kinect). Does NOT close the cameras, but updates only
    * the parts of the configuration which can be updated without restart (e.g., camera trafos and processing).
    *
    * \param configFile The pathname to the new camera configuration or an inline json configuration string.
    * \return A boolean that is true if the fast reload was successful.
    */
    virtual bool reload_config_fast(const char* configFile) = 0;

    /** \brief Return current configuration as a JSON string
    * 
    * For actual camera capturers this returns the current configuration in a buffer supplied by the caller.
    * 
    * \param buffer Where the JSON will be stored. Pass NULL to get the size of the buffer needed.
    * \param size Size of the buffer.
    */
    virtual size_t get_config(char* buffer, size_t size) = 0;
    /** \brief Start the source.
     * 
     * \return True if the source started successfully.
     */
    virtual bool start() = 0;

    /** \brief Stop the source.
     * 
     */

    virtual void stop() = 0;

    /** \brief Performs a seek on the playback.
     * \param timestamp The timestamp wanted
     * \return A boolean that is true if the seek was successful.
     *
     */
    virtual bool seek(uint64_t timestamp) = 0;

    /** \brief Return maximum number of possible tiles.
     *
     * Pointclouds produced by this source will (even after tile-processing)
     * never contain more tiles than this.
     * Note that this number used to contain potential fused cameras but no longer:
     * It is either 0 or 1 or nCamera+1.
     */
    virtual int maxtile() = 0;

    /** \brief Return information on a tile number.
     * \param tilenum The tile on which to obtain information.
     * \param tileinfo A pointer to a structure filled with information on the tile (if non-NULL).
     * \return A boolean that is true if the tile exists.
     *
     * Tile number 0 is a special case, representing the whole pointcloud.
     */
    virtual bool get_tileinfo(int tilenum, struct cwipc_tileinfo* tileinfo) = 0;


    /** \brief Request specific metadata to be added to pointclouds.
     * \param name Name of the metadata wanted.
     * 
     * Capturing metadata data (such as skeletons, or RGBD images) may be expensive, therefore
     * you need to request the specific metadata you need.
     *
     * If a subclass needs special handing for dynamic requests of metadata items
     * it should override this method, call the base and then use is_metadata_requested()
     * to see which data is currently wanted.
     */
    virtual void request_metadata(const std::string& name) {
        metadata_wanted.insert(name);
    }

    /** \brief Returns true is specific metadata has been requested
     * \param name Name of the metadata
     * \returns True or false
     */
    bool is_metadata_requested(const std::string& name) {
        return metadata_wanted.find(name) != metadata_wanted.end();
    }

    /** \brief Do an auxiliary operation.
     * \param op The operation to perform
     * \param inbuf Buffer with parameters to the operation
     * \param insize Size of inbuf
     * \param outbuf Buffer for results of the operation
     * \param outsize Size of outbuf
     * \returns success indicator.
     * 
     * Operations could be things like mapping 2D coordinates to 3D coordinates, or
     * getting camera metadata.
     */
    virtual bool auxiliary_operation(const std::string op, const void* inbuf, size_t insize, void* outbuf, size_t outsize) {
        return false;
    }

private:
    std::set<std::string> metadata_wanted;
};

/** \brief A consumer of pointclouds, abstract C++ interface.
 *
 * This interface is provided by renderers and such. It allows the
 * user of this interface to send cwipc pointcloud data somewhere.
 *
 */
class cwipc_sink {
public:
    /** \brief Destructor. Does not free underlying sink.
     * 
     * You must call free() before calling the destructor (unless you have passed
     * the cwipc object across an implementation language boundary and a reference
     * may still be held there).
     */ 
    virtual ~cwipc_sink() {};

    /** \brief Deallocate the pointcloud sink.
     *
     * Because the pointcloud sink may be used in a different implementation
     * language or DLL than where it is implemented we do not count on refcounting
     * and such. Call this method if you no longer need the source.
     */
    virtual void free() = 0;

    /** \brief Feed a pointcloud to the sink.
     * \param pc The pointcloud
     * \param clear If true a display window will clear any previous pointclouds
     * \return True if the operation was successful.
     *
     * A display sink will likely show the pointcloud in a window and give the
     * user some interaction commands to inspect it.
     *
     * Note that if the sink needs to keep the pointcloud data it will make a
     * copy (or shallow copy), so after feed() returns the caller still owns
     * the original point cloud and can reuse it or call free().
     */
    virtual bool feed(cwipc_pointcloud* pc, bool clear) = 0;

    /** \brief Set a caption or title on the window.
     * \param caption The UTF8 caption string.
     * \return True if this sink could present the caption to the user.
     *
     * If the sink is a display window this will set some sort of caption
     * on the window.
     */
    virtual bool caption(const char* caption) = 0;

    /** \brief User interaction.
     * \param prompt A prompt message to show to the user, explaining what the program wants.
     * \param responses A string with all characters that can be typed by the user.
     * \param millis The number of milliseconds to wait for interaction, 0 for no wait or -1 for forever.
     * \return The character typed by the user, or '\0' if this sink does not support user interaction.
     */
    virtual char interact(const char* prompt, const char* responses, int32_t millis) = 0;
};

/** \brief A collection of metadata.
 * 
 * This collection (which is freed along with the cwipc point cloud it belongs to) contains
 * all the information needed to access and parse the metadata items. It is the
 * return value of cwipc->access_metadata().
 */
class cwipc_metadata {
public:
    typedef void (*deallocfunc)(void*);

    virtual ~cwipc_metadata() {}

    /** \brief Returns number of metadata items
     * \returns Number of metadata items.
     */
    virtual int count() = 0;

    /** \brief Return name of an item
     * \param idx The item index
     * \return the name
     */
    virtual const std::string& name(int idx) = 0;

    /** \brief Return descrption of an item
     * \param idx The item index
     * \return the description
     *
     * The description is intended to be machine-readable for code that understands it.
     * For example, it will contain image width and height and such.
     */
    virtual const std::string& description(int idx) = 0;

    /** \brief Return data pointer of an item
     * \param idx The item index
     * \return the data pointer
     */
    virtual void* pointer(int idx) = 0;

    /** \brief Return size of an item
     * \param idx The item index
     * \return the size in bytes
     */
    virtual size_t size(int idx) = 0;

    /** \brief Add an metadata item (internal use only)
     * \param name The item name
     * \param description String describing the item format
     * \param pointer The item pointer
     * \param size The size of the item
     * \param dealloc The item deallocator function
     */
    virtual void _add(const std::string& name, const std::string& description, void* pointer, size_t size, deallocfunc dealloc) = 0;

    /** \brief Move all metadata items to another collection (internal use only)
     * \param other The collection to move the items to.
     *
     * All metadata items moved to another collection, and the current collection is cleared, so ownership of the items is passed to
     * the other collection.
     */
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

    /** \brief Return version string.
     */
    _CWIPC_UTIL_EXPORT const char *cwipc_get_version();

    /** \brief configure logging.
     * The environment variable CWIPC_LOGGING can also be used to set the log level.
     * Subsequent calls to this function will override previous settings.
     * Ensure to clear callback before cleanup when calling from other languages.
     * \param level The maximum log level in which we are interested. CWIPC_LOG_LEVEL_NONE keeps log-level as-is.
     * \param callback A callback function to receive log messages. If NULL, logging to callback is disabled and logging goes to stderr.
     */
    _CWIPC_UTIL_EXPORT void cwipc_log_configure(int level, cwipc_log_callback_t callback);

    /** \brief Emit a log message.
     *  Note that this function is primarily intended for use by cwipc extensions or wrappers.
     * \param level The log level of this message.
     * \param module The module emitting the log message.
     * \param message The log message.
     */
    _CWIPC_UTIL_EXPORT void _cwipc_log_emit(int level, const char* module, const char* message);

    /** \brief Return number of currently allocated pointcloud objects
     * \param log If memory debugging is enabled it will also emit log messages with information about them.
     */
    _CWIPC_UTIL_EXPORT int cwipc_dangling_allocations(bool log);

    /** \brief Read pointcloud from .ply file.
     * \param filename The ply file to read.
     * \param timestamp The timestamp to record in the cwipc object.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure dll compatibility.
     * \return the abstract point cloud, or NULL in case of errors.
     *
     * If an error occurs and errorMessage is non-NULL it will receive a pointer to
     * a string with the message.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_read(const char* filename, uint64_t timestamp, char** errorMessage, uint64_t apiVersion);

    /** \brief Write pointcloud to .ply file.
     * \param filename The ply file to frite.
     * \param pc The pointcloud to write.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \return 0 on success, -1 on failure.
     *
     * If an error occurs and errorMessage is non-NULL it will receive a pointer to
     * a string with the message.
     */
    _CWIPC_UTIL_EXPORT int cwipc_write(const char* filename, cwipc_pointcloud* pc, char** errorMessage);

    /** \brief Write pointcloud to .ply file.
     * \param filename The ply file to frite.
     * \param pc The pointcloud to write.
     * \param flag Pass CWIPC_FLAG_BINARY to write a binary file
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \return 0 on success, -1 on failure.
     *
     * If an error occurs and errorMessage is non-NULL it will receive a pointer to
     * a string with the message.
     */
    _CWIPC_UTIL_EXPORT int cwipc_write_ext(const char* filename, cwipc_pointcloud* pc, int flag, char** errorMessage);

    /** \brief Create cwipc pointcloud from external representation.
     * \param points Pointer to buffer with points.
     * \param size Size of points in bytes.
     * \param npoint Number of points (must match size).
     * \param timestamp The timestamp to record in the cwipc object.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure dll compatibility.
     * \return the abstract point cloud, or NULL in case of errors.
     *
     * If an error occurs and errorMessage is non-NULL it will receive a pointer to
     * a string with the message.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_from_points(struct cwipc_point* points, size_t size, int npoint, uint64_t timestamp, char** errorMessage, uint64_t apiVersion);

    /** \brief Create cwipc pointcloud from external representation.
     * \param packet Pointer to packet obtained from cwipc_pointcloud_copy_packet
     * \param size Size of points in bytes.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure dll compatibility.
     * \return the abstract point cloud, or NULL in case of errors.
     *
     * If an error occurs and errorMessage is non-NULL it will receive a pointer to
     * a string with the message.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_from_packet(uint8_t* packet, size_t size, char** errorMessage, uint64_t apiVersion);

    /** \brief Read pointcloud from pointclouddump file.
     * \param filename The dump file to read.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure dll compatibility.
     * \return the abstract point cloud, or NULL in case of errors.
     *
     * The dump file format is unspecified and machine dependent. It is mainly
     * intended for testing purposes.
     *
     * If an error occurs and errorMessage is non-NULL it will receive a pointer to
     * a string with the message.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_read_debugdump(const char* filename, char** errorMessage, uint64_t apiVersion);

    /** \brief Write pointcloud to pointclouddump file.
     * \param filename The dump file to frite.
     * \param pc The pointcloud to write.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \return 0 on success, -1 on failure.
     *
     * The dump file format is unspecified and machine dependent. It is mainly
     * intended for testing purposes.
     *
     * If an error occurs and errorMessage is non-NULL it will receive a pointer to
     * a string with the message.
     */
    _CWIPC_UTIL_EXPORT int cwipc_write_debugdump(const char* filename, cwipc_pointcloud* pc, char** errorMessage);


    /** \brief Deallocate the pointcloud data (C interface).
     * \param pc The cwipc_pointcloud object.
     *
     * The internal implementation of the pointcloud data may be refcounted, but
     * an explicit free method must be called to ensure the refcounting does not
     * inadvertantly free the pointcloud when dereferenced from a different DLL or
     * implementation language.
     *
     * Whoever created the cwipc object in the first place is responsible for
     * calling free, and the correct DLL will be invoked to actually free the data.
     */
    _CWIPC_UTIL_EXPORT void cwipc_pointcloud_free(cwipc_pointcloud* pc);

    /** \brief Create a shallow copy of a pointcloud object.
     * \param pc The cwipc_pointcloud object.
     * \return The new cwipc_pointcloud object.
     * The shallow copy shares the underlying (refcounted) point data storage.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud *cwipc_pointcloud__shallowcopy(cwipc_pointcloud* pc);
    
    /** \brief Time this pointcloud was captured (C interface).
     * \param pc The cwipc_pointcloud object.
     * \return Time in milliseconds, since some unspecified origin.
     */
    _CWIPC_UTIL_EXPORT uint64_t cwipc_pointcloud_timestamp(cwipc_pointcloud* pc);

    /** \brief Returns the grid cell size at which this pointcloud was created, if known (C interface).
     * \param pc The cwipc_pointcloud object.
     * \return Either a size (in the same coordinates as x, y and z) or 0 if unknown.
     *
     * If the pointcloud is to be displayed the number returned by this call is a good
     * guess for a pointsize to use to show an obect that doesn't have any holes in it.
     */
    _CWIPC_UTIL_EXPORT float cwipc_pointcloud_cellsize(cwipc_pointcloud* pc);

    /** \brief Semi-private method to initialize the cellsize (C interface). Not for general use.
     */
    _CWIPC_UTIL_EXPORT void cwipc_pointcloud__set_cellsize(cwipc_pointcloud* pc, float cellsize);

    /** \brief Semi-private method to initialize the timestamp. Not for general use.
     */
    _CWIPC_UTIL_EXPORT void cwipc_pointcloud__set_timestamp(cwipc_pointcloud* pc, uint64_t timestamp);

    /** \brief Returns number of points in the pointcloud (C interface).
     * \param pc The cwipc_pointcloud object.
     * \return The number of points.
     */
    _CWIPC_UTIL_EXPORT int cwipc_pointcloud_count(cwipc_pointcloud* pc);

    /** \brief Returns size (in bytes) an external representation of this pointcloud needs (C interface).
    * \param pc The cwipc_pointcloud object.
    * \return The number of bytes needed (or zero in case the format does not match
    *   or no points are available).
    */
    _CWIPC_UTIL_EXPORT size_t cwipc_pointcloud_get_uncompressed_size(cwipc_pointcloud* pc);

    /** \brief Get points from pointcloud in external representation format (C interface).
     * \param pc The cwipc_pointcloud object.
     * \param pointbuf A databuffer pointer.
     * \param size The size of the databuffer (in bytes).
     * \return The number of points.
     *
     * The caller is responsible for first calling get_uncompressed_size() and
     * then allocating the data. This is to ensure that the memory is allocated in
     * a way that is compatible with the caller (for example if the caller is
     * implemented in another language like C# or Python and needs a special
     * allocator).
     */
    _CWIPC_UTIL_EXPORT int cwipc_pointcloud_copy_uncompressed(cwipc_pointcloud* pc, struct cwipc_point* pointbuf, size_t size);

    /** \brief Get pointcloud in external representation format (C interface).
     * \param pc The cwipc_pointcloud object.
     * \param packet A databuffer pointer.
     * \param size The size of the databuffer (in bytes).
     * \return The size of the databuffer
     *
     * Call with packet=NULL to obtain packet buffer size. Then allocate a buffer and
     * call with buffer and size to copy the packet data.
     */
    _CWIPC_UTIL_EXPORT size_t cwipc_pointcloud_copy_packet(cwipc_pointcloud* pc, uint8_t* packet, size_t size);

    /** \brief Access metadata collection (C interface).
     * \param pc The cwipc_pointcloud object.
     * \return A reference to the metadata collection
     *
     * Note that this function returns a borrowed reference (and that the collection consists of more
     * borrowed references). AThese references become invalid when free() is called.
     */
    _CWIPC_UTIL_EXPORT cwipc_metadata* cwipc_pointcloud_access_metadata(cwipc_pointcloud* pc);

    /** \brief Start a pointcloud source
     * 
     * \param src The cwipc_activesource object.
     * \return True if the source started successfully.
    */
    _CWIPC_UTIL_EXPORT bool cwipc_activesource_start(cwipc_activesource* src);

    /** \brief Stop a pointcloud source
     * 
     * \param src The cwipc_activesource object.
    */
    _CWIPC_UTIL_EXPORT void cwipc_activesource_stop(cwipc_activesource* src);
    
    /** \brief Get a new pointcloud (C interface).
     * \param src The cwipc_source object.
     * \return The new pointcloud.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_source_get(cwipc_source* src);

    /** \brief Deallocate the pointcloud source (C interface).
     * \param src The cwipc_source object.
     *
     * Because the pointcloud source may be used in a different implementation
     * language or DLL than where it is implemented we do not count on refcounting
     * and such. Call this method if you no longer need the source.
     */
    _CWIPC_UTIL_EXPORT void cwipc_source_free(cwipc_source* src);

    /** \brief Return true if no more pointclouds are forthcoming (C interface).
     * \param src The cwipc_source object.
     */
    _CWIPC_UTIL_EXPORT bool cwipc_source_eof(cwipc_source* src);

    /** \brief Return true if a pointcloud is currently available (C interface).
     * \param src The cwipc_source object.
     * \param wait Set to true if the caller is willing to wait until a pointcloud is available.
     *
     * If this cwipc_source is not multi-threading capable the wait parameter is ignored.
     * If it is multi-threaded aware and no pointcloud is currently available
     * it may wait a reasonable amount of time (think: about a second) to see whether
     * one becomes available.
     */
    _CWIPC_UTIL_EXPORT bool cwipc_source_available(cwipc_source* src, bool wait);

    /** \brief Request specific metadata to be added to pointclouds (C interface).
     * \param src The cwipc_activesource object.
     * \param name Name of the metadata items wanted
     */
    _CWIPC_UTIL_EXPORT void cwipc_activesource_request_metadata(cwipc_activesource* src, const char* name);

    /** \brief Returns true is specific metadata has been requested (C interface).
     * \param src The cwipc_activesource object.
     * \param name Name of the metadata items
     * \returns True or false
     */
    _CWIPC_UTIL_EXPORT bool cwipc_activesource_is_metadata_requested(cwipc_activesource* src, const char* name);

    /** \brief Reload capturer based on a new configuration
    *
    * Only implemented for actual cameras (realsense, kinect). Closes the cameras and reopens them with a new
    * configuration.
    *
    * \param src The cwipc_activesource object.
     * \param configFile The pathname to the new camera configuration or an inline json configuration string.
    * \return A boolean that is true if the reload was successful.
    */
    _CWIPC_UTIL_EXPORT bool cwipc_activesource_reload_config(cwipc_activesource* src, const char* configFile);

    /** \brief Reload parts of the capturer based on a new configuration
    *
    * Only implemented for actual cameras (realsense, kinect). Does NOT close the cameras, but updates only
    * the parts of the configuration which can be updated without restart (e.g., camera trafos and processing).
    *
    * \param src The cwipc_activesource object.
     * \param configFile The pathname to the new camera configuration or an inline json configuration string.
    * \return A boolean that is true if the reload was successful.
    */
    _CWIPC_UTIL_EXPORT bool cwipc_activesource_reload_config_fast(cwipc_activesource* src, const char* configFile);

    /** \brief Return current configuration as a JSON string
    *
    * For actual camera capturers this returns the current configuration in a buffer supplied by the caller.
    *
    * \param src The cwipc_activesource object.
    * \param buffer Where the JSON will be stored. Pass NULL to get the size of the buffer needed.
    * \param size Size of the buffer.
    */
    _CWIPC_UTIL_EXPORT size_t cwipc_activesource_get_config(cwipc_activesource* src, char* buffer, size_t size);

    /** \brief Attempt to seek a a specific timestamp in a point cloud stream (C interface).
     * \param src The cwipc_activesource object.
     * \param timestamp The timestamp wanted.
     * \return True if successful.
     *
     */
    _CWIPC_UTIL_EXPORT bool cwipc_activesource_seek(cwipc_activesource* src, uint64_t timestamp);

    /** \brief Return maximum number of possible tiles returned (C interface).
     * \param src The cwipc_activesource object.
     *
     * Pointclouds produced by this source will (even after tile-processing)
     * never contain more tiles than this.
     * Note that this number may therefore be higher than the actual number of
     * tiles ever occurring in a pointcloud: a next tiling step may combine
     * points into new tiles.
     */
    _CWIPC_UTIL_EXPORT int cwipc_activesource_maxtile(cwipc_activesource* src);

    /** \brief Return information on a tile number (C interface).
     * \param src The cwipc_activesource object.
     * \param tilenum The tile on which to obtain information.
     * \param tileinfo A pointer to a structure filled with information on the tile (if non-NULL).
     * \return A boolean that is true if the tile could ever exist.
     *
     * Tile number 0 is a special case, representing the whole pointcloud.
     */
    _CWIPC_UTIL_EXPORT bool cwipc_activesource_get_tileinfo(cwipc_activesource* src, int tilenum, struct cwipc_tileinfo* tileinfo);

    /** \brief Do an auxiliary operation (C interface).
     * \param src The cwipc_activesource object.
     * \param op The operation to perform
     * \param inbuf Buffer with parameters to the operation
     * \param insize Size of inbuf
     * \param outbuf Buffer for results of the operation
     * \param outsize Size of outbuf
     * \returns success indicator.
     * 
     * Operations could be things like mapping 2D coordinates to 3D coordinates, or
     * getting camera metadata.
     */
    _CWIPC_UTIL_EXPORT bool cwipc_activesource_auxiliary_operation(cwipc_activesource *src, const char* op, const void* inbuf, size_t insize, void* outbuf, size_t outsize);


    /** \brief Deallocate the pointcloud sink (C interface).
     *
     * Because the pointcloud sink may be used in a different implementation
     * language or DLL than where it is implemented we do not count on refcounting
     * and such. Call this method if you no longer need the source.
     */
    _CWIPC_UTIL_EXPORT void cwipc_sink_free(cwipc_sink* sink);

    /** \brief Feed a pointcloud to the sink (C interface).
     * \param sink The cwipc_sink object.
     * \param pc The pointcloud
     * \param clear If true a display window will clear any previous pointclouds
     * \return True if the operation was successful.
     *
     * A display sink will likely show the pointcloud in a window and give the
     * user some interaction commands to inspect it.
     *
     * Note that if the sink needs to keep the pointcloud data it will make a
     * copy, so after feed() returns the caller can safely call pc.free().
     */
    _CWIPC_UTIL_EXPORT bool cwipc_sink_feed(cwipc_sink* sink, cwipc_pointcloud* pc, bool clear);

    /** \brief Set a caption or title on the window (C interface).
     * \param sink The cwipc_sink object.
     * \param caption The UTF8 caption string.
     * \return True if this sink could present the caption to the user.
     *
     * If the sink is a display window this will set some sort of caption
     * on the window.
     */
    _CWIPC_UTIL_EXPORT bool cwipc_sink_caption(cwipc_sink* sink, const char* caption);

    /** \brief User interaction (C interface).
     * \param sink The cwipc_sink object.
     * \param prompt A prompt message to show to the user, explaining what the program wants.
     * \param responses A string with all characters that can be typed by the user.
     * \param millis The number of milliseconds to wait for interaction, 0 for no wait or -1 for forever.
     * \return The character typed by the user, or '\0' if the user did not type anything,
     * or if this sink does not support user interaction.
     */
    _CWIPC_UTIL_EXPORT char cwipc_sink_interact(cwipc_sink* sink, const char* prompt, const char* responses, int32_t millis);

    /** \brief move all metadata from one collection to another
     * \param src source metadata collection
     * \param dest destination metadata collection
     */
    _CWIPC_UTIL_EXPORT void cwipc_metadata__move(cwipc_metadata*  src, cwipc_metadata* dest);
    
    /** \brief Returns number of items in the metadata collection (C interface).
     * \param collection the metadata collection
     * \returns Number of metadata items.
     */
    _CWIPC_UTIL_EXPORT int cwipc_metadata_count(cwipc_metadata* collection);

    /** \brief Returns name of an item in the collection (C interface).
     * \param collection the metadata collection
     * \param idx The item index
     * \returns Name (borrowed reference)
     */
    _CWIPC_UTIL_EXPORT const char* cwipc_metadata_name(cwipc_metadata* collection, int idx);

    /** \brief Return description of an item (C interface).
     * \param collection the metadata collection
     * \param idx The item index
     * \return the description (borrowed reference)
     *
     * The description is intended to be machine-readable for code that understands it.
     * For example, it will contain image widht and height and such.
     */
    _CWIPC_UTIL_EXPORT const char* cwipc_metadata_description(cwipc_metadata* collection, int idx);


    /** \brief Returns data pointer of an item in the collection (C interface).
     * \param collection the metadata collection
     * \param idx The item index
     * \returns Data pointer (borrowed reference)
     */
    _CWIPC_UTIL_EXPORT void* cwipc_metadata_pointer(cwipc_metadata* collection, int idx);

    /** \brief Returns size of a data item in the collection (C interface).
     * \param collection the metadata collection
     * \param idx The item index
     * \returns size
     */
    _CWIPC_UTIL_EXPORT size_t cwipc_metadata_size(cwipc_metadata* collection, int idx);

    /** \brief Generate synthetic pointclouds.
     * \param fps Maximum frames-per-second produced (0 for unlimited)
     * \param npoints Approximate number of points in pointcloud (0 for default 160Kpoint)
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure dll compatibility.
     *
     * This function returns a cwipc_source that generates a rotating pointcloud
     * of the object colloquially known as the colourful dildo. It is intended for testing
     * purposes.
     */
    _CWIPC_UTIL_EXPORT cwipc_activesource* cwipc_synthetic(int fps, int npoints, char** errorMessage, uint64_t apiVersion);

    /** \brief Capture pointclouds from a RGBD camera.
     * \param configFilename A string with the filename of the camera configuration file.
     * \param errorMessage An optional pointer to a string where any error message will be stored.
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure DLL compatibility.
     * \return A cwipc_source object.

     * configFilename can be the filename of either a JSON or XML cameraconfig file.
     * NULL means use cameraconfig.json in the current directory, "auto" will create a capturer for all cameras of
     * any type that are currently attached (and supported). In inline JSON-encoded configuration can also be passed.
     *
     * This function will use the camera type specified in the cameraconfig to create a capturer implementation for the
     * correct camera type.
     */
    _CWIPC_UTIL_EXPORT cwipc_activesource* cwipc_capturer(const char *configFilename, char** errorMessage, uint64_t apiVersion);

    /** \brief Display a window to show pointclouds.
     * \param title The title string, to be shown in the title bar of the window.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure dll compatibility.
     *
     * This function will open a window and returns a cwipc_sink. When the program
     * feeds pointclouds into the sink these will be displayed in the window.
     * The user has some interaction commands to inspect the pointcloud.
     *
     * Note that it may be necessary (depending on the operating system) to call this
     * function only from the main thread, and to call the methods on the cwipc_sink
     * returned also only from the main thread.
     */
    _CWIPC_UTIL_EXPORT cwipc_sink* cwipc_window(const char* title, char** errorMessage, uint64_t apiVersion);


    /** \brief Downsample a cwipc pointcloud.
     * \param pc The source pointcloud
     * \param voxelsize Wanted resolution for returned pointcloud.
     * \return a new pointcloud
     *
     * A grid of voxelsize*voxelsize*voxelsize is overlaid over the pointcloud. All
     * points within each cube are combined into the new point, with coordinates and colors
     * of that new point being a smart average of the original points in the cell.
     * The tile of the new point is the OR of the tiles of the contributing points.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_downsample(cwipc_pointcloud* pc, float voxelsize);

    /** \brief Remove outliers from a cwipc pointcloud.
     * \param pc The source pointcloud
     * \param kNeighbors number of neighbors to analyze for each point.
     * \param stddevMulThresh standard deviation multiplier
     * \param perTile bool to select applying the filter per tile or to the full pc
     * \return a cleaned pointcloud
     *
     * All points who have a distance larger than stddevMulThresh standard deviation of 
     * the mean distance to the query point will be marked as outliers and removed
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_remove_outliers(cwipc_pointcloud* pc, int kNeighbors, float stddevMulThresh, bool perTile);

    /** \brief Filter a pointcloud by tile.
     * \param pc The source pointcloud.
     * \param tile The tile number.
     * \return a new pointcloud
     *
     * Returns a new pointcloud (possibly empty) that consists only of the points
     * with the given tile number.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_tilefilter(cwipc_pointcloud* pc, int tile);

    /** \brief Change tiling of a pointcloud.
     * \param pc The source pointcloud.
     * \param map The tile number mapping.
     * \return a new pointcloud
     *
     * Returns a new pointcloud where the tile number of each point is mapped through `map`.
     * This can be used to combine tiles, creating a pointcloud with fewer tiles than the original
     * (for example by only keeping the "major" tiles, and combining all remaining tiles into a single one).
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_tilemap(cwipc_pointcloud* pc, uint8_t map[256]);

    /** \brief Spatially crop a pointcloud.
     * \param pc The source pointcloud.
     * \param bbox minx, maxx, miny, maxy, minz, maxz
     * \return a new pointcloud
     *
     * Returns a new cropped pointcloud.
     * The new pointcloud contains only the points that fall within the bbxo bounding box.
     * Lower bounds are included, upper bounds are excluded.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_crop(cwipc_pointcloud* pc, float bbox[6]);

    /** \brief Change colors in a pointcloud.
     * \param pc The source pointcloud.
     * \param clearBits Map of bits to clear in the ARGB color
     * \param setBits Map of bits to set in the ARGB color
     * \return a new pointcloud
     *
     * Returns a new pointcloud with the color of each point modified.
     * Note: this function is primarliy intended for debugging and inspecting pointclouds, it is not
     * coded for efficiency.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_colormap(cwipc_pointcloud* pc, uint32_t clearBits, uint32_t setBits);

    /** \brief Combine two pointclouds
     * \param pc1 A source pointcloud.
     * \param pc2 Another source pointcloud.
     * \return a new pointcloud
     *
     * Returns a new pointcloud that contains all points in both sources.
     * Timestamp and cellsize is set to the minimum of both values in the sources.
     * Note: this function is primarliy intended for debugging and inspecting pointclouds, it is not
     * coded for efficiency.
     */
    _CWIPC_UTIL_EXPORT cwipc_pointcloud* cwipc_join(cwipc_pointcloud* pc1, cwipc_pointcloud* pc2);

    /** \brief Receive pointclouds over a socket connection.
     * \param host Local hostname or IP address to bind socket to (default: 0.0.0.0)
     * \param port Local port number to bind socket to.
     * \param errorMessage Address of a char* where any error message is saved (or NULL).
     * \param apiVersion Pass in CWIPC_API_VERSION to ensure dll compatibility.
     *
     * This function creates a server (in a separate thread) that listens on the given port
     * for an incoming pointcloud stream. Those pointclouds are then returned similar as to
     * synthetic or normal grabbed pointclouds.
     */
    _CWIPC_UTIL_EXPORT cwipc_activesource* cwipc_proxy(const char* host, int port, char** errorMessage, uint64_t apiVersion);

#ifdef __cplusplus
}
#endif

#endif // _cwipc_util_api_h_
