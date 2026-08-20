#include <cstdlib>

// Define to get (a little) debug prints
#undef CWIPC_DEBUG
#undef CWIPC_DEBUG_THREAD

#include "K4ACamera.hpp"

K4ACamera::K4ACamera(Type_api_camera _handle, K4ACaptureConfig& configuration, K4ACaptureMetadataConfig& metadata, int _camera_index)
:   K4ABaseCamera("cwipc_kinect: K4ACamera", _handle, configuration, metadata, _camera_index)
{
    if (debug) _log_debug("Creating camera");
    if (configuration.record_to_directory != "") {
        record_to_file = configuration.record_to_directory + "/" + serial + ".mkv";
    }
    _init_filters();
}

// Configure and initialize caputuring of one camera
bool K4ACamera::start_camera() {
    assert(camera_stopped);
    assert(camera_capturer_thread == nullptr);
    assert(camera_processing_thread == nullptr);
    k4a_device_configuration_t device_config = K4A_DEVICE_CONFIG_INIT_DISABLE_ALL;
    if (debug) _log_debug("Starting pipeline");
    if (!_init_config_for_this_camera(device_config)) {
        return false;
    }
    // xxxjack this is where we should open camera_handle.

    // xxxjack this block is for fast pointcloud generation.
    if (K4A_RESULT_SUCCEEDED != k4a_device_get_calibration(camera_handle, device_config.depth_mode, device_config.color_resolution, &sensor_calibration)) {
        _log_error("k4a_device_get_calibration failed");
        return false;
    }

    depth_to_color_extrinsics = sensor_calibration.extrinsics[0][1];
    transformation_handle = k4a_transformation_create(&sensor_calibration);

    if (depth_uv_mapping == NULL) { // generate depth_uv_mapping for the fast pc_gen v2
        _create_depth_uv_mapping(&sensor_calibration);
    }
    // xxxjack end of block

    k4a_result_t res = k4a_device_start_cameras(camera_handle, &device_config);
    if (res != K4A_RESULT_SUCCEEDED) {
        _log_error("k4a_device_start_cameras failed");
        return false;
    }
     if (record_to_file != "") {
        k4a_record_t _recorder;
        _log_trace("starting recording to " + record_to_file);
        res = k4a_record_create(record_to_file.c_str(), camera_handle, device_config, &_recorder);
        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("k4a_record_create failed for file " + record_to_file);
            return false;
        }
        res = k4a_record_write_header(_recorder);
        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("k4a_record_write_header failed for file " + record_to_file);
            return false;
        }
        recorder = _recorder;
    }
    if (debug) _log_debug("camera started");

    // xxxjack rs2 has _post_start_this_camera()
    // xxxjack rs2 has _ComputePointSize()
    camera_started = true;
    return true;
}

void K4ACamera::stop_camera() {
    if (camera_stopped) {
        _log_warning("Camera already stopped");
        return;
    }
    if (debug) _log_debug("stop camera");

    camera_stopped = true;
    processing_frame_queue.try_enqueue(NULL);

    // Stop threads
    if (camera_capturer_thread) {
        camera_capturer_thread->join();
    }

    delete camera_capturer_thread;
    camera_capturer_thread = nullptr;

    if (camera_processing_thread) {
        camera_processing_thread->join();
    }

    delete camera_processing_thread;
    camera_processing_thread = nullptr;
#ifdef CWIPC_WITH_KINECT_SKELETONS
    if (tracker_handle) {
        //Stop body tracker
        k4abt_tracker_shutdown(tracker_handle);
        k4abt_tracker_destroy(tracker_handle);
        tracker_handle = nullptr;
    }
#endif
    if (camera_started) {
        // Stop camera
        k4a_device_stop_cameras(camera_handle);
        camera_started = false;
    }

    if (recorder) {
        auto res = k4a_record_flush(recorder);
        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("k4a_record_flush failed");
        }
        k4a_record_close(recorder);
        recorder = nullptr;
    }
    
    if (camera_handle) {
        k4a_device_close(camera_handle);
        camera_handle = nullptr;
    }

    // Delete objects
    if (current_captured_frameset != NULL) {
        k4a_capture_release(current_captured_frameset);
        current_captured_frameset = NULL;
    }

    if (transformation_handle) {
        k4a_transformation_destroy(transformation_handle);
        transformation_handle = NULL;
    }

    processing_done = true;
    processing_done_cv.notify_one();
    if (debug) _log_debug("camera stopped");
}

uint64_t K4ACamera::wait_for_captured_frameset(uint64_t minimum_timestamp) {
    
    k4a_capture_t new_frameset = NULL;
    uint64_t candidate_timestamp = 0;
    do {
        if (camera_stopped) {
            return 0;
        }
        bool rv = captured_frame_queue.wait_dequeue_timed(new_frameset, 5000000);

        if (!rv) {
            _log_warning("No frameset captured in 5 seconds");
            return 0;
        }
        if (new_frameset == nullptr) {
            _log_error("wait_for_captured_frameset: got nullptr");
        }
#ifdef _bad_kinect_timestamps
        // The Kinect gives two timestamps, both not really convenient:
        // - system timestamps (monotonic clock) of USB reception.
        // - device timestamps.
        // Both of these have an unspecified origin. And I don't think the device timestamps
        // are synchronized between devices (at least the docs don't say anything about this).
        //
        // Therefore we have disabled this code, and just use the system time.

        // We look at both the depth and color image timestamp.
        // We print a warning if they are not eqaul, and use the oldest.
        k4a_image_t depth_image = k4a_capture_get_depth_image(new_frameset);
        uint64_t ts_depth_image = k4a_image_get_system_timestamp_nsec(depth_image);
        k4a_image_release(depth_image);
        k4a_image_t color_image = k4a_capture_get_color_image(new_frameset);
        uint64_t ts_color_image = k4a_image_get_system_timestamp_nsec(color_image);
        k4a_image_release(color_image);
        uint64_t ts_system = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        if (debug) _log_debug("wait_for_captured_frameset: dts=" + std::to_string(ts_depth_image) + ", cts=" + std::to_string(ts_color_image));
        if (ts_depth_image != ts_color_image) {
            // xxxjack this comparison should have some leeway
            _log_warning("Frameset has color_ts=" + std::to_string(ts_color_image) + " depth_ts=" + std::to_string(ts_depth_image) + " system_ts=" + std::to_string(ts_system));
        }
        candidate_timestamp = ts_depth_image < ts_color_image ? ts_depth_image : ts_color_image;
#else
        uint64_t ts_system = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        candidate_timestamp = ts_system;
#endif
        if (candidate_timestamp < minimum_timestamp) {
            _log_warning("Frameset skipped, ts=" + std::to_string(candidate_timestamp) + ", earlier than " + std::to_string(minimum_timestamp));
        }
    } while (candidate_timestamp < minimum_timestamp);

    if (current_captured_frameset) {
        k4a_capture_release(current_captured_frameset);
    }

    current_captured_frameset = new_frameset;
    return candidate_timestamp;
}

bool K4ACamera::_init_config_for_this_camera(k4a_device_configuration_t& device_config) {
    device_config.color_format = hardware.color_mjpeg? K4A_IMAGE_FORMAT_COLOR_MJPG : K4A_IMAGE_FORMAT_COLOR_BGRA32;

    switch (hardware.color_height) {
    case 720:
        device_config.color_resolution = K4A_COLOR_RESOLUTION_720P;
        break;
    case 1080:
        device_config.color_resolution = K4A_COLOR_RESOLUTION_1080P;
        break;
    case 1440:
        device_config.color_resolution = K4A_COLOR_RESOLUTION_1440P;
        break;
    case 1536:
        device_config.color_resolution = K4A_COLOR_RESOLUTION_1536P;
        break;
    case 2160:
        device_config.color_resolution = K4A_COLOR_RESOLUTION_2160P;
        break;
    case 3072:
        device_config.color_resolution = K4A_COLOR_RESOLUTION_3072P;
        break;
    default:
        _log_error("invalid color_height: " + std::to_string(hardware.color_height));
        return false;
    }

    switch (hardware.depth_height) {
    case 288:
        device_config.depth_mode = K4A_DEPTH_MODE_NFOV_2X2BINNED;
        break;
    case 576:
        device_config.depth_mode = K4A_DEPTH_MODE_NFOV_UNBINNED;
        break;
    case 512:
        device_config.depth_mode = K4A_DEPTH_MODE_WFOV_2X2BINNED;
        break;
    case 1024:
        device_config.depth_mode = K4A_DEPTH_MODE_WFOV_UNBINNED;
        break;
    default:
        _log_error("invalid depth_height: " + std::to_string(hardware.depth_height));
        return false;
    }

    switch (hardware.fps) {
    case 5:
        device_config.camera_fps = K4A_FRAMES_PER_SECOND_5;
        break;
    case 15:
        device_config.camera_fps = K4A_FRAMES_PER_SECOND_15;
        break;
    case 30:
        device_config.camera_fps = K4A_FRAMES_PER_SECOND_30;
        break;
    default:
        _log_error("invalid camera_fps: " + std::to_string(hardware.fps));
        return false;
    }

    device_config.synchronized_images_only = true; // ensures that depth and color images are both available in the capture

    //SYNC:
    if (camera_sync_ismaster) {
        device_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_MASTER;
    } else if (camera_sync_inuse) {
        device_config.wired_sync_mode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
        device_config.subordinate_delay_off_master_usec = 160 * (camera_index+1); //160 allows max 9 cameras
    } else {
        // standalone mode, nothing to set
    }

    return true;
}


void K4ACamera::get_camera_hardware_parameters(K4ACameraHardwareConfig &output)
{
    // Some parameters are easiest to get from our config, as they won't
    // change anyway
    output.color_height = hardware.color_height;
    output.depth_height = hardware.depth_height;
    output.fps = hardware.fps;

    k4a_result_t res;
    k4a_color_control_mode_t mode;
    int32_t value;
    // color_exposure_time
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        if (mode == K4A_COLOR_CONTROL_MODE_AUTO) {
            output.color_exposure_time = -value;
        } else {
            output.color_exposure_time = value;
        }
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_exposure_time");
        output.color_exposure_time = hardware.color_exposure_time;
    }
    // color_whitebalance
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_WHITEBALANCE, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        if (mode == K4A_COLOR_CONTROL_MODE_AUTO) {
            output.color_whitebalance = -value;
        } else {
            output.color_whitebalance = value;
        }
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_whitebalance");
        output.color_exposure_time = hardware.color_exposure_time;
    }
    // color_backlight_compensation
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        if (mode == K4A_COLOR_CONTROL_MODE_AUTO) {
            output.color_exposure_time = -value;
        } else {
            output.color_exposure_time = value;
        }
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_exposure_time");
        output.color_exposure_time = hardware.color_exposure_time;
    }
    // color_backlight_compensation
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        output.color_backlight_compensation = value;
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_backlight_compensation");
        output.color_backlight_compensation = hardware.color_backlight_compensation;
    }
    // color_brightness
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_BRIGHTNESS, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        output.color_brightness = value;
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_brightness");
        output.color_brightness = hardware.color_brightness;
    }
    // color_contrast
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_CONTRAST, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        output.color_contrast = value;
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_contrast");
        output.color_contrast = hardware.color_contrast;
    }
    // color_saturation
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_SATURATION, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        output.color_saturation = value;
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_saturation");
        output.color_saturation = hardware.color_saturation;
    }
    // color_sharpness
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_SHARPNESS, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        output.color_sharpness = value;
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_sharpness");
        output.color_sharpness = hardware.color_sharpness;
    }
    // color_gain
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_GAIN, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        output.color_gain = value;
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_gain");
        output.color_gain = hardware.color_gain;
    }
    // color_powerline_frequency
    res = k4a_device_get_color_control(camera_handle, K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, &mode, &value);
    if (res == K4A_RESULT_SUCCEEDED) {
        output.color_powerline_frequency = value;
    } else {
        _log_warning("k4a_device_get_color_control: cannot get color_powerline_frequency");
        output.color_powerline_frequency = hardware.color_powerline_frequency;
    }
}
bool K4ACamera::_init_hardware_for_this_camera()
{
    // Set various camera hardware parameters (color)

    //options for color sensor
    if (hardware.color_exposure_time >= 0) { //MANUAL
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_exposure_time); // Exposure_time (in microseconds)

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration: k4a_device_set_color_control: color_exposure_time should be microsecond and in range (500-133330)");
            return false;
        }
    } else {  //AUTO
        k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_EXPOSURE_TIME_ABSOLUTE, K4A_COLOR_CONTROL_MODE_AUTO, 0);
    }

    if (hardware.color_whitebalance >= 0) {  //MANUAL
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_WHITEBALANCE, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_whitebalance); // White_balance (2500-12500)

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration: k4a_device_set_color_control: color_whitebalance should be in range (2500-12500)");
            return false;
        }
    } else {  //AUTO
        k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_WHITEBALANCE, K4A_COLOR_CONTROL_MODE_AUTO, 0);
    }

    if (hardware.color_backlight_compensation >= 0){
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_BACKLIGHT_COMPENSATION, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_backlight_compensation); // Backlight_compensation 0=disabled | 1=enabled. Default=0

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration: k4a_device_set_color_control: color_backlight_compensation should be 0=disabled | 1=enabled");
            return false;
        }
    }

    if (hardware.color_brightness >= 0){
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_BRIGHTNESS, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_brightness); // Brightness. (0 to 255). Default=128.

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_warning("configuration: k4a_device_set_color_control: color_brightness should be in range (0-255)");
            return false;
        }
    }

    if (hardware.color_contrast >= 0){
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_CONTRAST, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_contrast); // Contrast (0-10). Default=5

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration: k4a_device_set_color_control: color_contrast should be in range (0-10)");
            return false;
        }
    }

    if (hardware.color_saturation >= 0){
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_SATURATION, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_saturation); // saturation (0-63). Default=32

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration: k4a_device_set_color_control: color_saturation should be in range (0-63)");
            return false;
        }
    }

    if (hardware.color_sharpness >= 0){
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_SHARPNESS, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_sharpness); // Sharpness (0-4). Default=2

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration: k4a_device_set_color_control: color_sharpness should be in range (0-4)");
            return false;
        }
    }

    if (hardware.color_gain >= 0){ //if autoexposure mode=AUTO gain does not affect
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_GAIN, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_gain); // Gain (0-255). Default=0

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration: k4a_device_set_color_control: color_gain should be in range (0-255)");
            return false;
        }
    }

    if (hardware.color_powerline_frequency >= 0){
        k4a_result_t res = k4a_device_set_color_control(camera_handle, K4A_COLOR_CONTROL_POWERLINE_FREQUENCY, K4A_COLOR_CONTROL_MODE_MANUAL, hardware.color_powerline_frequency); // Powerline_Frequency (1=50Hz, 2=60Hz). Default=2

        if (res != K4A_RESULT_SUCCEEDED) {
            _log_error("configuration:k4a_device_set_color_control: color_powerline_frequency should be 1=50Hz or 2=60Hz");
            return false;
        }
    }
    return true;
}

void K4ACamera::_start_capture_thread()
{
    camera_capturer_thread = new std::thread(&K4ACamera::_capture_thread_main, this);
    _cwipc_setThreadName(camera_capturer_thread, L"cwipc_kinect::K4ACamera::capture_thread");
}

void K4ACamera::_capture_thread_main() {
    if (debug) _log_debug_thread("capture thread started");
    while(!camera_stopped) {
        k4a_capture_t capture_handle;
        if (k4a_capture_create(&capture_handle) != K4A_RESULT_SUCCEEDED) {
            _log_error("k4a_capture_create failed");

            break;
        }
        waiting_for_capture = true;
        k4a_wait_result_t ok = k4a_device_get_capture(camera_handle, &capture_handle, 5000);
        if (ok != K4A_WAIT_RESULT_SUCCEEDED) {
            _log_warning("k4a_device_get_capture failed");
            k4a_capture_release(capture_handle);

            continue;
        }

        if (capture_handle == nullptr) {
            if (camera_stopped) break;
            _log_error("k4a_device_get_capture return success, but capture is NULL");
            continue;
        }

        if (recorder != nullptr) {
            auto res = k4a_record_write_capture(recorder, capture_handle);
            if (res != K4A_RESULT_SUCCEEDED) {
                _log_warning("k4a_record_write_capture failed");
            }
        }
#ifdef CWIPC_DEBUG_THREAD
        k4a_image_t color = k4a_capture_get_color_image(capture_handle);
        uint64_t tsRGB = k4a_image_get_device_timestamp_usec(color);
        k4a_image_release(color);
        k4a_image_t depth = k4a_capture_get_depth_image(capture_handle);
        uint64_t tsD = k4a_image_get_device_timestamp_usec(depth);
        k4a_image_release(depth);
        if (debug) _log_debug_thread("captured frame " + std::to_string(tsRGB) + "/" + std::to_string(tsD) + " from camera " + serial);
#endif

        if (!captured_frame_queue.try_enqueue(capture_handle)) {
            // Queue is full. discard.
#ifdef CWIPC_DEBUG_THREAD
            k4a_image_t color = k4a_capture_get_color_image(capture_handle);
            uint64_t tsRGB = k4a_image_get_device_timestamp_usec(color);
            k4a_image_release(color);
            k4a_image_t depth = k4a_capture_get_depth_image(capture_handle);
            uint64_t tsD = k4a_image_get_device_timestamp_usec(depth);
            k4a_image_release(depth);
            if (debug) _log_debug_thread("dropped frame " + std::to_string(tsRGB) + "/" + std::to_string(tsD) + " from camera " + serial);
#endif
            k4a_capture_release(capture_handle);
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        } else {
            // Frame deposited in queue
            std::this_thread::sleep_for(std::chrono::milliseconds(25));
        }
    }

    if (debug) _log_debug_thread("capture thread stopping");
}
