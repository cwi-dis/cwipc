#include "OrbbecCamera.hpp"
#include "OrbbecConfig.hpp"

OrbbecCamera::OrbbecCamera(std::shared_ptr<ob::Device> camera, OrbbecCaptureConfig& config, OrbbecCaptureMetadataConfig& metadata, int cameraIndex)
: OrbbecBaseCamera("cwipc_orbbec: OrbbecCamera", camera, config, metadata, cameraIndex)
{
    if (config.record_to_directory != "") {
        record_to_file = config.record_to_directory + "/" + camera_config.serial + ".bag";
    }
}

bool OrbbecCamera::start_camera() {
    assert(camera_stopped);
    assert(!camera_started);
    assert(camera_processing_thread == nullptr);
    if (debug) _log_debug("Starting pipeline");
    auto config = std::make_shared<ob::Config>();
    if (!_init_pipeline_for_this_camera(config)) {
        return false;
    }
    try {
        camera_pipeline->start(config);
    } catch(ob::Error& e) {
        _log_error(std::string("pipeline.start error: ") + e.what());
      return false;
    }

    _post_start_this_camera();
    // xxxjack _computePointSize()??
    camera_started = true;
    return true;
}

void OrbbecCamera::_post_start_this_camera() {
    // XXX Implement me
}


bool OrbbecCamera::_init_hardware_for_this_camera() {
    if (hardware.color_exposure_time >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_EXPOSURE_INT, hardware.color_exposure_time);
    } else {
        camera_device->setBoolProperty(OB_PROP_COLOR_AUTO_EXPOSURE_BOOL, true);
    }

    if (hardware.color_whitebalance >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_WHITE_BALANCE_INT, hardware.color_whitebalance);
    } else {
        camera_device->setBoolProperty(OB_PROP_COLOR_AUTO_WHITE_BALANCE_BOOL, true);
    }

    if (hardware.color_backlight_compensation >= 0 && camera_device->isPropertySupported(OB_PROP_COLOR_BACKLIGHT_COMPENSATION_INT, OB_PERMISSION_WRITE)) {
        camera_device->setIntProperty(OB_PROP_COLOR_BACKLIGHT_COMPENSATION_INT, hardware.color_backlight_compensation);
    }

    if (hardware.color_brightness >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_BRIGHTNESS_INT, hardware.color_brightness);
    }

    if (hardware.color_contrast >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_CONTRAST_INT, hardware.color_contrast);
    }

    if (hardware.color_saturation >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_SATURATION_INT, hardware.color_saturation);
    }

    if (hardware.color_sharpness >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_SHARPNESS_INT, hardware.color_sharpness);
    }

    if (hardware.color_gain >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_GAIN_INT, hardware.color_gain);
    }

    if (hardware.color_powerline_frequency >= 0) {
        camera_device->setIntProperty(OB_PROP_COLOR_POWER_LINE_FREQUENCY_INT, hardware.color_powerline_frequency);
    }
    return true;
}

bool OrbbecCamera::_init_pipeline_for_this_camera(std::shared_ptr<ob::Config> config) {
    if (camera_device == nullptr) {
        _log_error("camera_device == nullptr");
        return false;
    }
    // Create the pipeline
    camera_pipeline = nullptr;
    camera_pipeline = std::make_shared<ob::Pipeline>(camera_device);
    // Ensure device clock is synchronized with host clock.
    camera_device->timerSyncWithHost();
    // Ensure frames are synchronized
    camera_pipeline->enableFrameSync();
    // Set a flag if we want to record.
    uses_recorder = record_to_file != "";
    try {
        config->enableVideoStream(OB_STREAM_COLOR, hardware.color_width, hardware.color_height, hardware.fps, OB_FORMAT_BGRA);
        config->enableVideoStream(OB_STREAM_DEPTH, hardware.depth_width, hardware.depth_height, hardware.fps, OB_FORMAT_Y16);
        config->setFrameAggregateOutputMode(OB_FRAME_AGGREGATE_OUTPUT_ALL_TYPE_FRAME_REQUIRE);
        // xxxjack this is really only for depth2color mode.
        config->setAlignMode(ALIGN_D2C_HW_MODE);
    } catch(ob::Error& e) {
        _log_error(std::string("enableVideoStream error: ") + e.what());
        return false;
    }
    return _start_recorder();
}

bool OrbbecCamera::_start_recorder() {
    if (!uses_recorder) return true;
    _log_trace("enabling recorder to " + record_to_file);
    recording_device = std::make_shared<ob::RecordDevice>(camera_device, record_to_file);
    if (recording_device == nullptr) {
        _log_error("Recorder failed for file: " + record_to_file);
        uses_recorder = false;
        return false;
    }
    return true;
}

void OrbbecCamera::_stop_recorder() {
    recording_device = nullptr;
}

void OrbbecCamera::start_camera_streaming() {
    assert(camera_stopped);
    camera_stopped = false;
    _start_capture_thread();
    _start_processing_thread();
}

void OrbbecCamera::_start_capture_thread() {
}

void OrbbecCamera::_capture_thread_main() {
}
