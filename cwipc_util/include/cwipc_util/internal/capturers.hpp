#ifndef _cwipc_util_capturers_hpp_
#define _cwipc_util_capturers_hpp_

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/logging.hpp"
#include <thread>

#define CWIPC_DEBUG
#define CWIPC_DEBUG_THREAD

#include <nlohmann/json.hpp>

//
// This file contains definitions of types and functions that are not user-visible,
// but that are provided by cwipc_util for use by other modules.
//

#ifdef _WIN32
#include <Windows.h>

inline void _cwipc_setThreadName(std::thread* thr, const wchar_t* name) {
    HANDLE threadHandle = static_cast<HANDLE>(thr->native_handle());
    SetThreadDescription(threadHandle, name);
}

#else
inline void _cwipc_setThreadName(std::thread* thr, const wchar_t* name) {}
#endif

using json = nlohmann::json;

#define _CWIPC_CONFIG_JSON_GET(jsonobj, name, config, attr) if (jsonobj.contains(#name)) jsonobj.at(#name).get_to(config.attr)
#define _CWIPC_CONFIG_JSON_PUT(jsonobj, name, config, attr) jsonobj[#name] = config.attr

/** Base class for per-camera configuration
 * 
 * Stores attributes common to all camera types, plus methods to serialize and deserialize.
 */
struct CwipcBaseCameraConfig {
    std::string type;
    bool disabled = false;      // to easily disable cameras without altering too much the cameraconfig
    bool connected = false;     // set to true when the camera is opened.
    std::string serial;         // Serial number of this camera
    std::string filename = "";     // filename, for playback
    pcl::shared_ptr<Eigen::Affine3d> trafo; //!< Transformation matrix from camera coorindates to world coordinates
    cwipc_vector cameraposition = { 0,0,0 };//!< Position of this camera in real world coordinates

    virtual void _from_json(const json& json_data) {
        json_data.at("type").get_to(type);
        if (json_data.contains("serial")) {
            json_data.at("serial").get_to(serial);
        }
        if (json_data.contains("disabled")) {
            json_data.at("disabled").get_to(disabled);
        }
        if (json_data.contains("filename")) {
            json_data.at("filename").get_to(filename);
        } else if (json_data.contains("playback_filename")) {
            // backwards compatibility
            json_data.at("playback_filename").get_to(filename);
        }
        // cameraposition is not serialized, it will be re-computed from the trafo.
        trafo = pcl::shared_ptr<Eigen::Affine3d>(new Eigen::Affine3d());
        if (json_data.contains("trafo")) {
            json trafo_json = json_data.at("trafo");
            for (int x = 0; x < 4; x++) {
                for (int y = 0; y < 4; y++) {
                    (*trafo)(x, y) = trafo_json[x][y];
                }
            }
        } else {
            trafo->setIdentity();
        }
        cameraposition.x = -(*trafo)(0, 3);
        cameraposition.y = -(*trafo)(1, 3);
        cameraposition.z = -(*trafo)(2, 3);
    };

    virtual void _to_json(json& json_data, bool for_recording=false) {
        json_data["type"] = type;
        json_data["serial"] = serial;
        json_data["disabled"] = disabled;
        if (filename != "") {
            json_data["filename"] = filename;
        }
        if (cameraposition.x != 0.0 || cameraposition.y != 0.0 || cameraposition.z != 0.0) {
            json_data["_cameraposition"] = {
                cameraposition.x,
                cameraposition.y,
                cameraposition.z
            };
        }
        json_data["trafo"] = {
            {(*trafo)(0, 0), (*trafo)(0, 1), (*trafo)(0, 2), (*trafo)(0, 3)},
            {(*trafo)(1, 0), (*trafo)(1, 1), (*trafo)(1, 2), (*trafo)(1, 3)},
            {(*trafo)(2, 0), (*trafo)(2, 1), (*trafo)(2, 2), (*trafo)(2, 3)},
            {(*trafo)(3, 0), (*trafo)(3, 1), (*trafo)(3, 2), (*trafo)(3, 3)}
        };
    };
};

/** Base class for capture configuration
 * 
 * Stores attributes common to all capturer types, plus methods to serialize and deserialize.
 */
struct CwipcBaseCaptureConfig {
    std::string type;

    virtual std::string to_string(bool for_recording=false) = 0;
    virtual bool from_string(const char* buffer, std::string typeWanted) = 0;
    virtual bool from_file(const char* filename, std::string typeWanted) = 0;
    virtual void _from_json(const json& json_data) {
        json_data.at("type").get_to(type);
    }
    virtual void _to_json(json& json_data, bool for_recording=false) {
        json_data["type"] = type;
        json_data["version"] = 5;
    }
};

/** Base class for both capturer and camera.
 * 
 * Only handles logging.
 */
class CwipcLoggingBase {
protected:
    std::string CLASSNAME;  //!< For error, warning and debug messages only
    CwipcLoggingBase(std::string _CLASSNAME)
    : CLASSNAME(_CLASSNAME)
    {}
    inline void _log(cwipc_log_level level, std::string message) {
        cwipc_log(level, CLASSNAME, message);
    }
public:
    inline void _log_error(std::string message) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, CLASSNAME, message);
    }
    inline void _log_warning(std::string message) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, CLASSNAME, message);
    }
    inline void _log_trace(std::string message) {
        cwipc_log(CWIPC_LOG_LEVEL_TRACE, CLASSNAME, message);
    }
    inline void _log_debug(std::string message) {
#ifdef CWIPC_DEBUG
        cwipc_log(CWIPC_LOG_LEVEL_DEBUG, CLASSNAME, message);
#endif
    }
    inline void _log_debug_thread(std::string message) {
#ifdef CWIPC_DEBUG_THREAD
        cwipc_log(CWIPC_LOG_LEVEL_DEBUG, CLASSNAME + " (thread)", message);
#endif
    }
};
/** Base class for camera 
 * 
*/
class CwipcBaseCamera : public CwipcLoggingBase {
protected:
    std::string type;

public:
    CwipcBaseCamera(std::string _CLASSNAME, std::string _type)
    : CwipcLoggingBase(_CLASSNAME),
      type(_type)
    {}
    virtual ~CwipcBaseCamera() {}
public:
    /// Step 1 in starting: tell the camera we are going to start. Called for all cameras.
    virtual bool pre_start_all_cameras() = 0;
    /// Step 2 in starting: starts the camera. Called for all cameras. 
    virtual bool start_camera() = 0;
    /// Step 3 in starting: starts the capturer. Called after all cameras have been started.
    virtual void start_camera_streaming() = 0;
    /// Step 4, called after all capturers have been started.
    virtual void post_start_all_cameras() = 0;
    
    /// Prepare for stopping the cameras. May do something like stopping the recording.
    virtual void pre_stop_camera() = 0;
    /// Completely stops camera and capturer, releases all resources. Can be re-started with start_camera, etc.
    virtual void stop_camera() = 0;
    /// Return true if this camera is the sync master.
    virtual bool is_sync_master() = 0;

    /// Seek. Fails for cameras, overriden for playback cameras.
    virtual bool seek(uint64_t timestamp) = 0;
    /// Are we at end-of-file? Always false for cameras.
    virtual bool eof() = 0;

    /// Map 2D color image coordinates to 2D depth image coordinates.
    virtual bool mapcolordepth(int x_c, int y_c, int *out2d) = 0;
    /// Map 2D color image coordinates to 3D coordinates.
    virtual bool map2d3d(int x_2d, int y_2d, int d_2d, float* out3d) = 0;
protected:
    // internal API that is "shared" with other implementations (realsense, kinect)
    /// Initialize any hardware settings for this camera.
    /// Also see xxxjack
    virtual bool _init_hardware_for_this_camera() = 0;
    /// Initialize any filters that will be applied to all RGB/D images.
    virtual bool _init_filters() = 0;
    /// Apply filter to a frameset.
    /// virtual void _apply_filters(...) = 0;
    /// Initialize the body tracker
    virtual bool _init_skeleton_tracker() = 0;
    /// Create per-API configuration for starting the camera 
    /// virtual void _init_pipeline_for_this_camera(...) = 0;
protected:
    /// Helper function to check whether a point is within a given radius from the Y=0 axis.
    inline bool isPointInRadius(cwipc_pcl_point& pt, float radius_filter) {
        float distance_2 = pow(pt.x, 2) + pow(pt.z, 2);
        return distance_2 < radius_filter * radius_filter; // radius^2 to avoid sqrt
    }

    /// Helper type: HSV color.
    typedef struct HsvColor {
        unsigned char h;
        unsigned char s;
        unsigned char v;
    } HsvColor;

    /// Helper function to convert RGB from a PCL point to HSV.
    inline HsvColor rgbToHsv(cwipc_pcl_point* pnt) {
        HsvColor hsv;
        unsigned char rgbMin, rgbMax;

        rgbMin = pnt->r < pnt->g ? (pnt->r < pnt->b ? pnt->r : pnt->b) : (pnt->g < pnt->b ? pnt->g : pnt->b);
        rgbMax = pnt->r > pnt->g ? (pnt->r > pnt->b ? pnt->r : pnt->b) : (pnt->g > pnt->b ? pnt->g : pnt->b);

        hsv.v = rgbMax;
        if (hsv.v == 0) {
            hsv.h = 0;
            hsv.s = 0;

            return hsv;
        }

        hsv.s = 255 * ((long)(rgbMax - rgbMin)) / hsv.v;
        if (hsv.s == 0) {
            hsv.h = 0;
            return hsv;
        }

        if (rgbMax == pnt->r) {
            hsv.h = 0 + 43 * (pnt->g - pnt->b) / (rgbMax - rgbMin);
        } else if (rgbMax == pnt->g) {
            hsv.h = 85 + 43 * (pnt->b - pnt->r) / (rgbMax - rgbMin);
        } else {
            hsv.h = 171 + 43 * (pnt->r - pnt->g) / (rgbMax - rgbMin);
        }

        return hsv;
    }

    /// Helper function to determine whether a point is not green (for greenscreen removal).
    inline bool isNotGreen(cwipc_pcl_point* p) {
        HsvColor hsv = rgbToHsv(p);

        if (hsv.h >= 60 && hsv.h <= 130) {
            if (hsv.s >= 0.15 && hsv.v >= 0.15) {
                // reducegreen
                if ((p->r * p->b) != 0 && (p->g * p->g) / (p->r * p->b) > 1.5) {
                    p->r *= 1.4;
                    p->b *= 1.4;
                } else {
                    p->r *= 1.2;
                    p->b *= 1.2;
                }
            }

            return !(hsv.s >= 0.4 && hsv.v >= 0.3);
        }

        return true;
    }

};

/** Base class for capturer 
 * 
*/
class CwipcBaseCapture : public CwipcLoggingBase {
protected:
    std::string type;   //!< cwipc type string, such as "kinect" or "realsense_playback"
public:
    /// Subclasses need to implement static factory(). 
    ///  It creates a new capturer instance.

    /// Subclasses need to implement static count_devices(). 
    ///  It counts the number of hardware devices acccessible to this machine.

    CwipcBaseCapture(std::string _CLASSNAME, std::string _type)
    : CwipcLoggingBase(_CLASSNAME),
      type(_type)
    {}
    virtual ~CwipcBaseCapture() { };

    /// Return the number of cameras connected to this capturer. Return 0 if something went wrong during initialization.
    virtual int get_camera_count() = 0;
    /// Return true if the cameras can be started, i.e. if initialization was successful.
    virtual bool can_start() = 0;
    /// Return a boolean stating whether the capturer is working (which implies it has cameras attached)
    virtual bool is_playing() = 0;
    /// Reload configuration.
    virtual bool config_reload(const char* configFilename) = 0;
    /// Fast reload configuration.
    virtual bool config_reload_fast(const char* configFilename) = 0;
    /// Start the capturer.
    virtual bool start() = 0;
    /// Stop capturing.
    virtual void stop() = 0;
    /// Get complete current configuration as JSON string.
    virtual std::string config_get() = 0;
    /// Request specific metadata to be added to pointclouds.
    virtual void request_metadata(bool rgb, bool depth, bool timestamps, bool skeleton, bool camera_specs) = 0;

    //
    // This section has the public capturer-independent API used during normal runtime.
    //

    /// Returns true when a new point cloud is available.
    virtual bool pointcloud_available(bool wait) = 0;
    /// Returns the new point cloud. The caller is now the owner of this point cloud.
    virtual cwipc_pointcloud* get_pointcloud() = 0;
    /// Returns a reasonable point size for the current capturer.
    virtual float get_pointSize() = 0;
    /// Return 3D point for a given camera, given RGB image 2D coordinates.
    virtual bool map2d3d(int tile, int x_2d, int y_2d, int d_2d, float* out3d) = 0;
    /// Return 2D point in depth image coordinates given 2D point in color image coordinates.
    virtual bool mapcolordepth(int tile, int u, int v, int* out2d) = 0;
    /// Return true if end-of-file has been reached (only for playback capturers).
    virtual bool eof() = 0;
    /// Seek to given timestamp (only implemented for playback capturers).
    virtual bool seek(uint64_t timestamp) = 0;
protected:
    /// Load configuration from file or string.
    virtual bool _apply_config(const char* configFilename) = 0;
    /// Load default configuration based on hardware cameras connected.
    virtual bool _apply_auto_config() = 0;
    /// Get configuration for a single camera, by serial number.
    /// Cannot do: Type_our_camera_config* get_camera_config(std::string serial)
    
    /// Setup camera synchronization (if needed).
    virtual bool _setup_inter_camera_sync() = 0;
    /// xxxjack another one?
    virtual void _initial_camera_synchronization() = 0;

    /// Create the per-camera capturers.
    virtual bool _create_cameras() = 0;
    /// Setup camera hardware parameters (white balance, etc).
    virtual bool _init_hardware_for_all_cameras() = 0;
    /// Check that all cameras are connected.
    virtual bool _check_cameras_connected() = 0;
    /// Start all cameras.
    virtual bool _start_cameras() = 0;
    
    /// Stop and unload all cameras and release all resources.
    virtual void _unload_cameras() = 0;
    /// Stop all cameras.
    virtual void _stop_cameras() = 0;
    /// When recording, create the cameraconfig.json file for the recording.
    virtual void _post_stop_all_cameras() = 0;
    
};

/** Template base class for capturer implementations
 * 
 * Two template parameters:
 * - GrabberClass: The class that actually implements the capturing
 * - CameraConfigClass: The class that implements per-camera configuration storage
 * Most implementations will have an extra layer of inheritance, to implement
 * common functionality provided by the API that is implemented both for live
 * cameras and for playback.
 */

template<class GrabberClass, class CameraConfigClass>
class cwipc_capturer_impl_base : public cwipc_activesource {
protected:
    GrabberClass *m_grabber; 
public:
    cwipc_capturer_impl_base(const char* configFilename) 
    : m_grabber(GrabberClass::factory())
    {
        m_grabber->config_reload(configFilename);
    }

    virtual ~cwipc_capturer_impl_base() {
        delete m_grabber;
        m_grabber = NULL;
    }

    virtual bool start() override final {
        return this->m_grabber->start();
    }

    virtual void stop() override final {
        this->m_grabber->stop();
    }

    virtual bool can_start() final {
        return m_grabber->can_start();
    }
    
    virtual bool is_playing() final {
        return m_grabber->is_playing();
    }

    virtual void free() override final {
        delete m_grabber;
        m_grabber = NULL;
    }

    virtual size_t get_config(char* buffer, size_t size) override final
    {
        auto config = m_grabber->config_get();

        if (buffer == nullptr) {
            return config.length();
        }

        if (size < config.length()) {
            return 0;
        }

        memcpy(buffer, config.c_str(), config.length());
        return config.length();
    }

    virtual bool reload_config(const char* configFile) override final {
        return m_grabber->config_reload(configFile);
    }

    virtual bool reload_config_fast(const char* configFile) override final {
        return m_grabber->config_reload_fast(configFile);
    }

    bool eof() override final {
        return m_grabber->eof();
    }

    bool available(bool wait) override final {
        if (m_grabber == NULL) {
            return false;
        }

        return m_grabber->pointcloud_available(wait);
    }

    cwipc_pointcloud* get() override final {
        if (m_grabber == NULL) {
            return NULL;
        }

        cwipc_pointcloud* rv = m_grabber->get_pointcloud();
        return rv;
    }

    int maxtile() override final {
        if (m_grabber == NULL) {
            return 0;
        }

        int nCamera = m_grabber->configuration.all_camera_configs.size();
        if (nCamera <= 1) {
            // Using a single camera or no camera.
            return nCamera;
        }

        return nCamera + 1;
    }

    bool get_tileinfo(int tilenum, struct cwipc_tileinfo *tileinfo) override final {
        if (m_grabber == NULL) {
            return false;
        }

        int nCamera = m_grabber->configuration.all_camera_configs.size();

        if (nCamera == 0) { // No camera
            return false;
        }

        if (tilenum < 0 || tilenum >= nCamera+1) {
            return false;
        }
        if (tilenum == 0) {
            // Special case: the whole pointcloud
            if (tileinfo) {
                tileinfo->normal = { 0, 0, 0 };
                tileinfo->cameraName = NULL;
                tileinfo->ncamera = nCamera;
                tileinfo->cameraMask = 0; // All cameras contributes to this
            }
            return true;
        }
        CameraConfigClass &cameraConfig = m_grabber->configuration.all_camera_configs[tilenum-1];
        if (tileinfo) {
            tileinfo->normal = cameraConfig.cameraposition; // Use the camera position as the normal
            tileinfo->cameraName = (char *)cameraConfig.serial.c_str();
            tileinfo->ncamera = 1; // Only one camera contributes to this
            tileinfo->cameraMask = (uint8_t)1 << (tilenum-1); // Only this camera contributes
        }
        return true;
    }
    
    virtual void request_metadata(const std::string &name) override = 0;
    virtual bool auxiliary_operation(const std::string op, const void* inbuf, size_t insize, void* outbuf, size_t outsize) override = 0;
    virtual bool seek(uint64_t timestamp) override = 0;
};

/** Capturer registration.
 * Capturer implementations should register themselves by calling
 * _cwipc_register_capturer() during their initialization.
 */
class cwipc_activesource;

typedef int _cwipc_functype_count_devices();
typedef cwipc_activesource* _cwipc_func_capturer_factory(const char *configFilename, char **errorMessage, uint64_t apiVersion);

extern "C" {
    _CWIPC_UTIL_EXPORT int _cwipc_register_capturer(const char* name, _cwipc_functype_count_devices* countFunc, _cwipc_func_capturer_factory* factoryFunc);
}
#endif // _cwipc_util_capturers_hpp_
