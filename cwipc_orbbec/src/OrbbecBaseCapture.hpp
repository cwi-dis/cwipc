#pragma once

#include <string>
#include <thread>
#include <condition_variable>
#include <iostream>
#include <fstream>
#include <pcl/common/transforms.h>

#define CWIPC_DEBUG
#define CWIPC_DEBUG_THREAD
#include "cwipc_util/internal/capturers.hpp"
#include "OrbbecConfig.hpp"

template<class Type_api_camera, class Type_our_camera> class OrbbecBaseCapture : public CwipcBaseCapture {
public:
    using CwipcBaseCapture::CwipcBaseCapture;


    static void _orbbec_message_cb(OBLogSeverity level, const char *message) {
        std::string orbbec_level = "";
        cwipc_log_level cwipc_level = CWIPC_LOG_LEVEL_WARNING;
        switch(level) {
        case OB_LOG_SEVERITY_FATAL:
            cwipc_level = CWIPC_LOG_LEVEL_ERROR;
            orbbec_level = "(fatal): ";
            break;
        case OB_LOG_SEVERITY_ERROR:
            cwipc_level = CWIPC_LOG_LEVEL_ERROR;
            break;
        case OB_LOG_SEVERITY_WARN:
            cwipc_level = CWIPC_LOG_LEVEL_WARNING;
            break;
        case OB_LOG_SEVERITY_INFO:
            cwipc_level = CWIPC_LOG_LEVEL_TRACE;
            break;
        case OB_LOG_SEVERITY_DEBUG:
            cwipc_level = CWIPC_LOG_LEVEL_DEBUG;
            break;
        default:
            cwipc_level = CWIPC_LOG_LEVEL_WARNING;
            orbbec_level = "(unknown-orbbec-level): ";
            break;
        }
        std::string msg = orbbec_level + message;
        cwipc_log(cwipc_level, "orbbec_sdk",msg );
    }

    static void _install_orbbec_logger() {
        ob::Context::setLoggerToCallback(OB_LOG_SEVERITY_INFO, _orbbec_message_cb);
    }

    static void _uninstall_orbbec_logger() {
        ob::Context::setLoggerToConsole(OB_LOG_SEVERITY_ERROR);
    }

    virtual ~OrbbecBaseCapture() {
        _unload_cameras();
        if (mergedPC) {
            mergedPC->free();
            mergedPC = nullptr;
        }
    }

    virtual bool can_start() override final {
        return _is_initialized;
    }

    virtual bool start() override final {
        if (!_is_initialized) {
            _log_error("start() called but not initialized");
            return false;
        }
        if (is_playing()) {
            _log_warning("start() called but already started");
            return false;
        }
        //
        // Initialize hardware capture setting (for all cameras)
        //
        if (!_init_hardware_for_all_cameras()) {
            return false;
        }

        _setup_inter_camera_sync();

        // Now we have all the configuration information. Create our K4ACamera objects.
        if (!_create_cameras()) {
            _unload_cameras();
            return false;
        }

        if (!_check_cameras_connected()) {
            _unload_cameras();
            return false;
        }
        if (!_start_cameras()) {
            _unload_cameras();
            return false;
        }

        //
        // start our run thread (which will drive the capturers and merge the pointclouds)
        //

        stopped = false;
        control_thread = new std::thread(&OrbbecBaseCapture::_control_thread_main, this);
        _cwipc_setThreadName(control_thread, L"cwipc_orbbec::control_thread");
        return true;
    }

    virtual void stop() override final {
        _unload_cameras();
    }   

    virtual int get_camera_count() override final { 
        return cameras.size(); 
    }

    virtual bool is_playing() override final { 
        return control_thread != nullptr;
    }

    virtual bool config_reload(const char* configFilename) override final{
        if (control_thread != nullptr) {
            _log_error("config_reload: cannot reload configuration while capturer is running");
            return false;
        }
        //
        // Read the configuration.
        //
        if (!_apply_config(configFilename)) {
            _is_initialized = false;
            return false;
        }
        if (cwipc_log_get_level() >= CWIPC_LOG_LEVEL_DEBUG) {
            configuration.debug = true;
        }
        if (configuration.apiDebug) {
            _install_orbbec_logger();
        } else {
            _uninstall_orbbec_logger();
        }
        
        auto camera_config_count = configuration.all_camera_configs.size();
        if (camera_config_count == 0) {
            return false;
        }
        _is_initialized = true;
        return true;
    }

    virtual bool config_reload_fast(const char* configFilename) override final {
        _log_error("xxxjack: pbauszat hasn't implemented config_reload_fast yet");
        return false;
    }

    virtual std::string config_get() override {
        if (cameras.size() == 0) {
            _log_error("Must start() before getting config");
            return "";
        }
        return configuration.to_string();
    }

    /// Tell the capturer that each point cloud should also include RGB and/or D images and/or RGB/D capture timestamps.
    virtual void request_metadata(bool rgb, bool depth, bool timestamps, bool skeleton, bool camera_specs ) override final {
        metadata.want_rgb = rgb;
        metadata.want_depth = depth;
        if (camera_specs) {
            _log_warning("xxxjack: pbauszat hasn't implemented camera_specs yet");
        }
    }

    virtual bool pointcloud_available(bool wait) override final {
        if (!is_playing()) {
            _log_warning("pointcloud_available() called but not playing");
            std::this_thread::sleep_for(std::chrono::seconds(1));
            return false;
        }
        _request_new_pointcloud();
        std::this_thread::yield();
        if(configuration.debug) _log_debug_thread("02. available: wait for fresh");
        std::unique_lock<std::mutex> mylock(mergedPC_mutex);
        auto duration = std::chrono::seconds(wait ? 1 : 0);
        mergedPC_is_fresh_cv.wait_for(mylock, duration, [this] {
            return mergedPC_is_fresh;
        });
        if(configuration.debug) _log_debug_thread("03. available: wait for fresh returned " + std::to_string(mergedPC_is_fresh));
        return mergedPC_is_fresh;
    }

    virtual cwipc_pointcloud* get_pointcloud() override final {
        if (!is_playing()) {
            _log_error("get_pointcloud: not playing");
            return nullptr;
        }
        _request_new_pointcloud();
        // Wait for a fresh mergedPC to become available.
        // Note we keep the return value while holding the lock, so we can start the next grab/process/merge cycle before returning.
        cwipc_pointcloud* rv;

        {
            if(configuration.debug) _log_debug_thread("02. get_pointcloud: wait for fresh");
            std::unique_lock<std::mutex> mylock(mergedPC_mutex);
            mergedPC_is_fresh_cv.wait(mylock, [this] {
                return mergedPC_is_fresh;
            });
            if(configuration.debug) _log_debug_thread("03. get_pointcloud: wait for fresh returned " + std::to_string(mergedPC_is_fresh));

            assert(mergedPC_is_fresh);
            mergedPC_is_fresh = false;
            numberOfPCsProduced++;
            rv = mergedPC;
            mergedPC = nullptr;

            if (rv == nullptr) {
                _log_warning("get_pointcloud: returning NULL, even though mergedPC_is_fresh");
            }
        }

        _request_new_pointcloud();
        return rv;
    }

    virtual float get_pointSize() override final {
        if (!is_playing()) {
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

    virtual bool map2d3d(int tile, int x_2d, int y_2d, int d_2d, float* out3d) override final
    {
        for (auto cam : cameras) {
            if (tile == (1 << cam->camera_index)) {
                return cam->map2d3d(x_2d, y_2d, d_2d, out3d);
            }
        }
        return false;
    }
    
    virtual bool mapcolordepth(int tile, int u, int v, int* out2d) override final
    {
        // For Kinect the RGB and D images have the same coordinate system.
        out2d[0] = u;
        out2d[1] = v;
        return true;
    }

    bool eof() override {
        return _eof;
    }

    virtual bool seek(uint64_t timestamp) override = 0;

protected:
    /// Load configuration from file or string.
    virtual bool _apply_config(const char* configFilename) override {
        OrbbecCaptureConfig newConfiguration;
        configuration = newConfiguration;
        if (configFilename == 0 || *configFilename == '\0') {
            configFilename = "cameraconfig.json";
        }

        if (strcmp(configFilename, "auto") == 0) {
            return _apply_auto_config();
        }

        if (configFilename[0] == '{') {
            return configuration.from_string(configFilename, type);
        }

        const char* extension = strrchr(configFilename, '.');
        if (extension != 0 && strcmp(extension, ".json") == 0) {
            return configuration.from_file(configFilename, type);
        } else {
            _log_error("Unknown configuration file type: '" + std::string(configFilename) + "'");
        }

        return false;
    }
    /// Load default configuration based on hardware cameras connected.
    virtual bool _apply_auto_config() override = 0;
    /// Get configuration for a single camera, by serial number.
    OrbbecCameraConfig* get_camera_config(std::string serial) {
        for (int i = 0; i < configuration.all_camera_configs.size(); i++) {
            if (configuration.all_camera_configs[i].serial == serial) {
                return &configuration.all_camera_configs[i];
            }
        }

        _log_error("Unknown camera " + serial);
        return 0;
    }

    /// Setup camera synchronization (if needed).
    virtual bool _setup_inter_camera_sync() override final {
        // Nothing to do for K4A: real cameras need some setup, but it is done
        // in K4ACamera::_prepare_config_for_starting_camera().
        return true;
    }
    /// xxxjack another one?
    virtual void _initial_camera_synchronization() override {
    }

    virtual bool _start_cameras() final {
        bool start_error = false;
        for (auto cam: cameras) {
            if (!cam->pre_start_all_cameras()) {
                start_error = true;
            }
        }
        if (!start_error) {
            // xxxjack Check for Orbbec. K4A wants master camera started _last_, because then
            // any recordings will be automatically starting at the same frame. Need to check
            // that this is also true for Orbbec.
            for (auto cam : cameras) {
                if (cam->is_sync_master()) {
                    continue;
                }

                if (!cam->start_camera()) {
                    start_error = true;
                }
            }

            for (auto cam : cameras) {
                if (!cam->is_sync_master()) {
                    continue;
                }

                if (!cam->start_camera()) {
                    start_error = true;
                }
            }
        }
        if (!start_error) {
            for (auto cam : cameras) {
                cam->post_start_all_cameras();
            }
        }
        if (start_error) {
            // xxxjack may override earlier error...
            _log_error("Not all cameras could be started");
            return false;
        }

        starttime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        for (auto cam : cameras) {
            if (cam->is_sync_master()) {
                continue;
            }

            cam->start_camera_streaming();
        }

        for (auto cam : cameras) {
            if (!cam->is_sync_master()) {
                continue;
            }

            cam->start_camera_streaming();
        }
        return true;
    }

    virtual void _unload_cameras() override final {

        if (cameras.empty()) return;
        _stop_cameras();

        for (auto cam : cameras) {
            delete cam;
        }

        cameras.clear();
        if(configuration.debug) _log_debug("deleted all cameras");
    }

    virtual void _stop_cameras() override final {
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
        control_thread = 0;
        if (configuration.debug) _log_debug_thread("stopped control thread");

        if (configuration.debug) _log_debug("stopping all cameras");
        for (auto cam : cameras) {
            cam->stop_camera();
        }

        mergedPC_is_fresh = false;
        mergedPC_want_new = false;
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

protected:

    void _control_thread_main() {
        if (configuration.debug) _log_debug("control thread started");
        _initial_camera_synchronization();
        while (!stopped) {
            if(configuration.debug) _log_debug_thread("1. wait for mergedPC_want_new");
            {
                std::unique_lock<std::mutex> mylock(mergedPC_mutex);
                mergedPC_want_new_cv.wait(mylock, [this] { return mergedPC_want_new; });
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
            if(configuration.debug) _log_debug_thread("2. capture_all_cameras()");
            assert(cameras.size() > 0);
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

            if (configuration.new_timestamps) {
                timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
            }
            if(configuration.debug) _log_debug_thread("3. create new pointcloud");
            // Step 2 - Create pointcloud, and save rgb/depth images if wanted
            if (configuration.debug) _log_debug("creating pc with ts=" + std::to_string(timestamp));
            cwipc_pcl_pointcloud pcl_pointcloud = new_cwipc_pcl_pointcloud();
            cwipc_pointcloud* newPC = cwipc_from_pcl(pcl_pointcloud, timestamp, nullptr, CWIPC_API_VERSION);


            for (auto cam : cameras) {
                cam->save_frameset_metadata(newPC);
            }

            if (stopped) {
                newPC->free();
                break;
            }
            if(configuration.debug) _log_debug_thread("4. all cameras->process_pointcloud_from_frameset()");
            // Step 3: start processing frames to pointclouds, for each camera
            for (auto cam : cameras) {
                cam->process_pointcloud_from_frameset();
            }

            if (stopped) {
                newPC->free();
                break;
            }
            if(configuration.debug) _log_debug_thread("5. wait  for mergedpc_mutex");
            // Lock mergedPC already while we are waiting for the per-camera
            // processing threads. This so the main thread doesn't go off and do
            // useless things if it is calling available(true).
            std::unique_lock<std::mutex> mylock(mergedPC_mutex);
            if (mergedPC && mergedPC_is_fresh) {
                mergedPC->free();
                mergedPC = nullptr;
            }

            mergedPC = newPC;
            if (stopped) {
                break;
            }

            if(configuration.debug) _log_debug_thread("6. all cameras->wait_for_pointcloud_processed()");
            // Step 4: wait for frame processing to complete.
            for (auto cam : cameras) {
                cam->wait_for_pointcloud_processed();
            }

            if (stopped) {
                break;
            }
            if(configuration.debug) _log_debug_thread("7. merge_camera_pointclouds()");
            // Step 5: merge views
            _merge_camera_pointclouds();

            if (mergedPC->access_pcl_pointcloud()->size() > 0) {
                if(configuration.debug) _log_debug("merged pointcloud has  " + std::to_string(mergedPC->access_pcl_pointcloud()->size()) + " points");
            } else {
                _log_warning("merged pointcloud is empty");
            }
            if(configuration.debug) _log_debug_thread("8. notify merged_pc_is_fresh. All done.");
            // Signal that a new mergedPC is available. (Note that we acquired the mutex earlier)
            mergedPC_is_fresh = true;
            mergedPC_want_new = false;
            mergedPC_is_fresh_cv.notify_all();
        }
        if (configuration.debug) _log_debug_thread("control thread exiting");
    }


    bool _capture_all_cameras(uint64_t& timestamp) {
        // xxxjack does not take master into account
        // xxxjack different from kinect playback.
        // The code here presumes that the hardware cameras will be more-or-less synchronized:
        // We start getting any frame from the first camera (first_timestamp is 0),
        // then for subsequent cameras we ensure we don't get a frame with a timestamp that is earlier.
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
            if(configuration.debug) _log_debug_thread("00. request new pointcloud");
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
    OrbbecCaptureConfig configuration;
    OrbbecCaptureMetadataConfig metadata;
protected:
    std::vector<Type_our_camera*> cameras;
    bool _is_initialized = false;
    bool stopped = false;
    bool _eof = false;

    uint64_t starttime = 0;
    int numberOfPCsProduced = 0;

    cwipc_pointcloud* mergedPC = nullptr;
    std::mutex mergedPC_mutex;

    bool mergedPC_is_fresh = false;
    std::condition_variable mergedPC_is_fresh_cv;

    bool mergedPC_want_new = false;
    std::condition_variable mergedPC_want_new_cv;

    std::thread* control_thread = 0;
};
