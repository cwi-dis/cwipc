#ifndef cwipc_realsense_RS2BaseCapture_hpp
#define cwipc_realsense_RS2BaseCapture_hpp
#pragma once


#include <thread>
#include <mutex>
#include <condition_variable>
#include <iostream>
#include <fstream>

#include <librealsense2/rs.hpp>

#include "cwipc_util/internal/capturers.hpp"

#include "RS2Config.hpp"


/** Base class for capturers that use the librealsense API. 
 * 
 * For librealsense actually most of the implementation is in this class,
 * because a playback device has the same API as a live camera.
 * 
 * Subclasses need to implement factory() and count_devices().
*/
template <class Type_our_camera, class Type_our_camera_config>
class RS2BaseCapture : public CwipcBaseCapture {
public:
    /// Subclasses need to implement static factory().
    /// Subclasses need to implement static count_devices().

    using CwipcBaseCapture::CwipcBaseCapture;
    virtual ~RS2BaseCapture()  {
        _unload_cameras();
        if(mergedPC) {
            mergedPC->free();
            mergedPC = nullptr;
        }
    }

    virtual bool can_start() override final {
        return _is_initialized;
    }

    virtual bool is_playing() override final {
        return control_thread != nullptr; 
    }

    virtual bool start() override final {
        if (!can_start()) {
            _log_error("start() called but not initialized");
            return false;
        }
        if (is_playing()) {
            _log_warning("start() called but already started");
            return false;
        }
        auto camera_config_count = configuration.all_camera_configs.size();
        if (camera_config_count == 0) {
            return false;
        }

        // Set various camera hardware parameters (white balance and such)
        _init_hardware_for_all_cameras();

        //
        // Set sync mode, if needed
        //
        _setup_inter_camera_sync();

        // Now we have all the configuration information. Open the cameras.
        // This will _not_ include the disabled cameras, so after this we should
        // look at cameras.size() instead of camera_config_count.
        if (!_create_cameras()) {
            _unload_cameras();
            return false;
        }
        if (!_check_cameras_connected()) {
            _unload_cameras();
            return false;
        }

        _start_cameras();

        //
        // start our run thread (which will drive the capturers and merge the pointclouds)
        //
        stopped = false;
        stopping = false;
        control_thread = new std::thread(&RS2BaseCapture::_control_thread_main, this);
        _cwipc_setThreadName(control_thread, L"cwipc_realsense2::RS2BaseCapture::control_thread");

        return true;
    }

    virtual void stop() override final{
        _unload_cameras();
    }   

    virtual int get_camera_count() override final { 
        return cameras.size(); 
    }

    virtual bool config_reload(const char *configFilename) override final {
        if (control_thread != nullptr) {
            _log_error("config_reload: cannot reload configuration while capturer is running");
            return false;
        }

        if (!_apply_config(configFilename)) {
            _is_initialized = false;
            return false;
        }
        if (cwipc_log_get_level() >= CWIPC_LOG_LEVEL_DEBUG) {
            configuration.debug = true;
        }
        _is_initialized = true;
        return true;
    }

    virtual bool config_reload_fast(const char* configFilename) override final {
        // Only reloads what is safe to reload without re-starting the cameras.
        // Only configuration files or inline JSON is supported here, no auto.

        RS2CaptureConfig new_configuration;
        bool success = false;
        if (configFilename != nullptr) {
            if (configFilename[0] == '{') {
                // Read from JSON string
                success = new_configuration.from_string(configFilename, type);
            }
            else {
                // Check file extension
                const char* extension = strrchr(configFilename, '.');
                if (extension != nullptr && strcmp(extension, ".json") == 0) {
                    // Try to read from file
                    success = new_configuration.from_file(configFilename, type);
                }
            }
        }

        //
        // If loading of the new configuration was successful copy over the suitable parts.
        //

        if (success) {
            // Copy over processing configuration (should be safe)
            configuration.processing = new_configuration.processing;

            // Copy over camera transforms (camera count must match!)
            if (configuration.all_camera_configs.size() == new_configuration.all_camera_configs.size()) {
                for (size_t i = 0; i < new_configuration.all_camera_configs.size(); ++i) {
                    configuration.all_camera_configs[i].trafo = new_configuration.all_camera_configs[i].trafo;
                    configuration.all_camera_configs[i].cameraposition = new_configuration.all_camera_configs[i].cameraposition;
                }
            }
            else {
                _log_error("Invalid configuration for fast reload (RS2) (size of cameras changed): '" + std::string(configFilename) + "'");
            }
        }
        else {
            _log_error("Invalid configuration for fast reload (RS2) (either not file or invalid JSON): '" + std::string(configFilename) + "'");
        }

        return success;
    }

    virtual std::string config_get() override final {
        if (cameras.size() == 0) {
            _log_error("Must start() before getting config");
            return "";
        }
        
        // We get the hardware parameters from the first camera.
        RS2CameraHardwareConfig curHardwareConfig;
        cameras[0]->get_camera_hardware_parameters(curHardwareConfig);
        configuration.hardware = curHardwareConfig;
#if 0
        for(auto cam : cameras) {
            bool ok = cam->match_camera_hardware_parameters(curHardwareConfig);
            if (!ok) {
                _log_warning("Not all cameras have the same hardware parameters.");
            }
            match_only = true;
        }
#endif
        return configuration.to_string();
    }

    /// Tell the capturer that each point cloud should also include RGB and/or D images and/or RGB/D capture timestamps.
    virtual void request_metadata(bool rgb, bool depth, bool timestamps, bool skeleton, bool camera_specs) override final {
        metadata.want_rgb = rgb;
        metadata.want_depth = depth;
        metadata.want_timestamps = timestamps;
        metadata.want_camera_specs = camera_specs;
    }

    //
    // This section has the public capturer-independent API used during normal runtime.
    //

    /// Returns true when a new point cloud is available.
    bool pointcloud_available(bool wait) override final {
        if (!is_playing()) {
            _log_warning("available() called but not playing");
            return false;
        }

        _request_new_pointcloud();

        std::this_thread::yield();
        std::unique_lock<std::mutex> mylock(mergedPC_mutex);

        auto duration = std::chrono::seconds(wait?1:0);
        mergedPC_is_fresh_cv.wait_for(mylock, duration, [this]{
            return mergedPC_is_fresh;
        });

        return mergedPC_is_fresh;
    }

    /// Returns the new point cloud. The caller is now the owner of this point cloud.
    cwipc_pointcloud* get_pointcloud() override final {
        if (!is_playing()) {
            _log_error("get_pointcloud: not playing");
          return nullptr;
        }

        _request_new_pointcloud();

        // Wait for a fresh mergedPC to become available.
        // Note we keep the return value while holding the lock, so we can start the next grab/process/merge cycle before returning.
        cwipc_pointcloud *rv;

        {
            std::unique_lock<std::mutex> mylock(mergedPC_mutex);

            mergedPC_is_fresh_cv.wait(mylock, [this] {
                return mergedPC_is_fresh;
            });

            mergedPC_is_fresh = false;
            rv = mergedPC;
        }

        _request_new_pointcloud();
        return rv;
    }

    /// Returns a reasonable point size for the current capturer.
    float get_pointSize() override final  {
        if (!is_playing()) {
            // xxxjack should we log a warning here?
            return 0;
        }

        float rv = 99999;
        for (auto cam : cameras) {
            if (cam->pointSize < rv) {
                rv = cam->pointSize;
            }
        }

        if (rv > 9999) {
            rv = 0;
        }

        return rv;
    }

    /// Return 3D point for a given camera, given RGB image 2D coordinates.
    virtual bool map2d3d(int tile, int x_2d, int y_2d, int d_2d, float* out3d) override final {
        for(auto cam : cameras) {
            if (tile == (1 << cam->camera_index)) {
                return cam->map2d3d(x_2d, y_2d, d_2d, out3d);
            }
        }
        return false;
    }

    /// Return 2D point in depth image coordinates given 2D point in color image coordinates.
    virtual bool mapcolordepth(int tile, int u, int v, int* out2d) override final {
        for(auto cam : cameras) {
            if (tile == (1 << cam->camera_index)) {
                return cam->mapcolordepth(u, v, out2d);
            }
        }
        return false;
    }
    virtual bool eof() override final {
        return _eof;
    };
    /// Seek to given timestamp (only implemented for playback capturers).
    virtual bool seek(uint64_t timestamp) override = 0;
   
protected:
    /// Load configuration from file or string.
    /// Note: not final, because some subclasses need to extend it.
    virtual bool _apply_config(const char* configFilename) override {
        // Clear out old configuration
        RS2CaptureConfig newConfiguration;
        configuration = newConfiguration;

        //
        // Read the configuration. We do this only now because for historical reasons the configuration
        // reader is also the code that checks whether the configuration file contents match the actual
        // current hardware setup. To be fixed at some point.
        //
        if (configFilename == NULL || *configFilename == '\0') {
            // Empty config filename: use default cameraconfig.json.
            configFilename = "cameraconfig.json";
        }

        if (strcmp(configFilename, "auto") == 0) {
            // Special case 1: string "auto" means auto-configure all realsense cameras.
            return _apply_auto_config();
        }

        if (configFilename[0] == '{') {
            // Special case 2: a string starting with { is considered a JSON literal
            return configuration.from_string(configFilename, type);
        }

        // Otherwise we check the extension. It can be .json.
        const char *extension = strrchr(configFilename, '.');
        if (extension != nullptr && strcmp(extension, ".json") == 0) {
            return configuration.from_file(configFilename, type);
        } else {
            _log_error("Unknown configuration file type: '" + std::string(configFilename) + "'");
        }

        return false;
    }
    /// Load default configuration based on hardware cameras connected.
    virtual bool _apply_auto_config() override = 0;
    /// Get configuration for a single camera, by serial number.
    Type_our_camera_config* get_camera_config(std::string serial) {
        for (int i = 0; i < configuration.all_camera_configs.size(); i++) {
            if (configuration.all_camera_configs[i].serial == serial) {
                return &configuration.all_camera_configs[i];
            }
        }

        _log_warning("Unknown camera " + serial);
        return nullptr;
    }

    
    /// Setup camera synchronization (if needed).
    virtual bool _setup_inter_camera_sync() override = 0;
    /// xxxjack another one?
    virtual void _initial_camera_synchronization() override {
    }

    /// Create the per-camera capturers.
    virtual bool _create_cameras() override = 0;
    /// Setup camera hardware parameters (white balance, etc).
    virtual bool _init_hardware_for_all_cameras() override = 0;
    /// Check that all cameras are connected.
    virtual bool _check_cameras_connected() override = 0;
    /// Start all cameras.
    virtual bool _start_cameras() override final {
        bool start_error = false;
        for (auto cam: cameras) {
            if (!cam->pre_start_all_cameras()) {
                start_error = true;
            }
        }
        if (!start_error) {
            try {
                for (auto cam: cameras) {
                    if (!cam->start_camera()) {
                        start_error = true;
                    }
                }
            } catch(const rs2::error& e) {
                _log_error("Exception while starting camera: " + e.get_failed_function() + ": " + e.what());
                start_error = true;
            }
        }
        if (start_error) {
            _log_error("Not all cameras could be started");
            return false;
        }
        //
        // start the per-camera capture threads
        //
        for (auto cam: cameras) {
            cam->start_camera_streaming();
        }
        
        for (auto cam: cameras) {
            cam->post_start_all_cameras();
        }
        return true;
    }
    
    /// Stop and unload all cameras and release all resources.
    virtual void _unload_cameras() override final {

        _stop_cameras();

        // Delete all cameras
        for (auto cam : cameras) {
            delete cam;
        }
        cameras.clear();
        if (configuration.debug) _log_debug("deleted all cameras");
    }

    /// Stop all cameras.
    virtual void _stop_cameras() override final {
        if (configuration.debug) _log_debug("pre-stopping all cameras");
        stopping = true;
        for (auto cam : cameras) {
            cam->pre_stop_camera();
        }
        if (configuration.debug) _log_debug("stopping all cameras");

        // Stop all cameras
        for (auto cam : cameras) {
            cam->stop_camera();
        }
        if (configuration.debug) _log_debug_thread("stopping control thread");
        stopped = true;
        mergedPC_is_fresh = true;
        mergedPC_want_new = false;
        mergedPC_is_fresh_cv.notify_all();
        mergedPC_want_new = true;
        mergedPC_want_new_cv.notify_all();


        if (control_thread && control_thread->joinable()) {
            control_thread->join();
        }

        delete control_thread;
        control_thread = nullptr;
        if (configuration.debug) _log_debug_thread("stopped control thread");
        _post_stop_all_cameras();
        if (configuration.debug) _log_debug("post-stopped");

    }

    /// Create the cameraconfig file for the recording, if needed.
    virtual void _post_stop_all_cameras() override final {
        if (configuration.record_to_directory != "") {
            std::string recording_config = configuration.to_string(true);
            std::string filename = configuration.record_to_directory + "/" + "cameraconfig.json";
            std::ofstream ofp;
            ofp.open(filename);
            ofp << recording_config << std::endl;
            ofp.close();
        }
    }
    
    void _control_thread_main()  {
        if (configuration.debug) _log_debug_thread("control thread started");
        _initial_camera_synchronization();
        while(!stopped && !stopping) {
            {
                std::unique_lock<std::mutex> mylock(mergedPC_mutex);
                mergedPC_want_new_cv.wait(mylock, [this]{
                    return mergedPC_want_new;
                });
            }
            //check EOF:
            for (auto cam : cameras) {
                if (cam->end_of_stream_reached) {
                    _eof = true;
                    stopped = true;
                    break;
                }
            }

            if (stopped) {
                break;
            }

            assert (cameras.size() > 0);

            // Step one: grab frames from all cameras. This should happen as close together in time as possible,
            // because that gives use he biggest chance we have the same frame (or at most off-by-one) for each
            // camera.
            uint64_t timestamp = 0;
            bool all_captures_ok = _capture_all_cameras(timestamp);

            if (!all_captures_ok) {
                std::this_thread::yield();
                continue;
            }
            if (stopped) {
                break;
            }

            // If we invent new timestamps, do it now.
            if (configuration.new_timestamps) {
                timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            }

            if (configuration.debug) _log_debug("creating pc with ts=" + std::to_string(timestamp));
            // step 2 : create pointcloud, and save rgb/depth frames if wanted

            cwipc_pcl_pointcloud pcl_pointcloud = new_cwipc_pcl_pointcloud();
            cwipc_pointcloud* newPC = cwipc_from_pcl(pcl_pointcloud, timestamp, NULL, CWIPC_API_VERSION);

            for (auto cam : cameras) {
                cam->save_frameset_metadata(newPC);
            }
            
            if (stopped) {
                newPC->free();
                break;
            }

            // Step 3: start processing frames to pointclouds, for each camera
            for(auto cam : cameras) {
                cam->process_pointcloud_from_frameset();
            }

            if (stopped) {
                newPC->free();
                break;
            }

            // Lock mergedPC already while we are waiting for the per-camera
            // processing threads. This so the main thread doesn't go off and do
            // useless things if it is calling available(true).
            std::unique_lock<std::mutex> mylock(mergedPC_mutex);
            if (mergedPC && mergedPC_is_fresh) {
                mergedPC->free();
                mergedPC = nullptr;
            }
            mergedPC = newPC;
            
            if (stopped) break;
            
            // Step 4: wait for frame processing to complete.
            for(auto cam : cameras) {
                cam->wait_for_pointcloud_processed();
            }

            // Step 5: merge views
            _merge_camera_pointclouds();

            if (mergedPC->access_pcl_pointcloud()->size() > 0) {
                if (configuration.debug) _log_debug("merged pointcloud has " + std::to_string(mergedPC->access_pcl_pointcloud()->size()) + " points");
            } else {
                if (configuration.debug) _log_debug("merged pointcloud is empty");
            }
            // Signal that a new mergedPC is available. (Note that we acquired the mutex earlier)
            mergedPC_is_fresh = true;
            mergedPC_want_new = false;
            mergedPC_is_fresh_cv.notify_all();
        }

        if (configuration.debug) _log_debug_thread("control thread exiting");
    }

    /// Anything that needs to be done to get the camera streams synchronized after opening.
    /// (Realsense Playback seeks all streams to the same timecode, the earliest one present
    /// in each stream)



    virtual bool _capture_all_cameras(uint64_t& timestamp) final {
        // xxxjack does not take master into account
        // xxxjack different from kinect
        uint64_t first_timestamp = 0;
        for(auto cam : cameras) {
            uint64_t this_cam_timestamp = cam->wait_for_captured_frameset(first_timestamp);
            if (cam->end_of_stream_reached) return false;
            if (this_cam_timestamp == 0) {
                _log_warning("no frameset captured from camera " + cam->serial);
                return false;
            }
            if (first_timestamp == 0) {
                first_timestamp = this_cam_timestamp;
            }
        }

        // And get the best timestamp
        if (configuration.new_timestamps) {
            timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        } else {
            timestamp = first_timestamp;
        }
        return true;
    }


    void _request_new_pointcloud() {
        std::unique_lock<std::mutex> mylock(mergedPC_mutex);

        if (!mergedPC_want_new && !mergedPC_is_fresh) {
            mergedPC_want_new = true;
            mergedPC_want_new_cv.notify_all();
        }
    }

    void _merge_camera_pointclouds() {
        cwipc_pcl_pointcloud aligned_cld(mergedPC->access_pcl_pointcloud());
        aligned_cld->clear();
        // Pre-allocate space in the merged pointcloud
        size_t nPoints = 0;

        for (auto cam : cameras) {
            cwipc_pcl_pointcloud cam_cld = cam->access_current_pcl_pointcloud();

            if (cam_cld == 0) {
                _log_warning("_merge_camera_pointclouds: camera pointcloud is null for some camera" );
                continue;
            }
            nPoints += cam_cld->size();
        }

        aligned_cld->reserve(nPoints);

        // Now merge all pointclouds
        for (auto cam : cameras) {
            cwipc_pcl_pointcloud cam_cld = cam->access_current_pcl_pointcloud();

            if (cam_cld == NULL) {
                continue;
            }

            *aligned_cld += *cam_cld;
        }
        if (aligned_cld->size() != nPoints) {
            _log_error("Combined pointcloud has different number of points than expected");
        }

        // No need to merge metadata: already inserted into mergedPC by each camera
    }    
public:
    /// Current configuration. Has to be public because cwipc_realsense2 needs access to all sorts of internals
    /// of it, but it would be better if this access was readonly...
    RS2CaptureConfig configuration;
    RS2CaptureMetadataConfig metadata;
    
protected:
    rs2::context capturer_context;

    std::vector<Type_our_camera*> cameras;    ///< The per-camera capturers
    bool _is_initialized = false;
    bool stopping = false;
    bool stopped = false;
    bool _eof = false;

    cwipc_pointcloud* mergedPC = nullptr;          ///< Merged pointcloud
    std::mutex mergedPC_mutex;          ///< Lock for all mergedPC-related dta structures
    
    bool mergedPC_is_fresh = false;     ///< True if mergedPC contains a freshly-created pointcloud
    std::condition_variable mergedPC_is_fresh_cv;   ///< Condition variable for signalling freshly-created pointcloud
    
    bool mergedPC_want_new = false;     ///< Set to true to request a new pointcloud
    std::condition_variable mergedPC_want_new_cv;   ///< Condition variable for signalling we want a new pointcloud

    std::thread *control_thread = nullptr;
    
};

#endif