#include "OrbbecPlaybackCamera.hpp"
#include "OrbbecConfig.hpp"

OrbbecPlaybackCamera::OrbbecPlaybackCamera(std::shared_ptr<ob::PlaybackDevice> camera, OrbbecCaptureConfig& config, OrbbecCaptureMetadataConfig& metadata, int cameraIndex, std::string filename)
:   OrbbecBaseCamera("cwipc_orbbec: OrbbecPlaybackCamera", camera, config, metadata, cameraIndex),
    playback_filename(filename)
{
}

bool OrbbecPlaybackCamera::start_camera() {
    assert(camera_stopped);
    assert(!camera_started);
    assert(camera_processing_thread == nullptr);
    assert(camera_pipeline == nullptr);
    playback_eof = false;
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

void OrbbecPlaybackCamera::_post_start_this_camera() {
    // XXX Implement me
    playback_device->setPlaybackStatusChangeCallback(
        [&](OBPlaybackStatus status) {
            _log_debug("PlaybackStatusCallback status=" + std::to_string((int)status));
            if (status == OB_PLAYBACK_STOPPED) {
                _stopped_callback();
            }
        });
}

void OrbbecPlaybackCamera::_stopped_callback() {
    _log_trace("end of file reached");
    playback_eof = true;
}

void OrbbecPlaybackCamera::pause() {

}

void OrbbecPlaybackCamera::resume() {

}

bool OrbbecPlaybackCamera::seek(uint64_t timestamp) {
    return false;
}

bool OrbbecPlaybackCamera::eof() {
    return playback_eof;
}

bool OrbbecPlaybackCamera::_init_pipeline_for_this_camera(std::shared_ptr<ob::Config> config) {
    // Create the playback device
    playback_device = std::make_shared<ob::PlaybackDevice>(playback_filename);
    camera_device = playback_device;
    // Create the pipeline
    camera_pipeline = nullptr;
    camera_pipeline = std::make_shared<ob::Pipeline>(camera_device);
#if 0
    // xxxjack is this retimestamp?
    // Ensure device clock is synchronized with host clock.
    camera_device->timerSyncWithHost();
#endif
    // Ensure frames are synchronized
    camera_pipeline->enableFrameSync();
    try {
        config->enableVideoStream(OB_STREAM_COLOR, hardware.color_width, hardware.color_height, hardware.fps, OB_FORMAT_BGRA);
        config->enableVideoStream(OB_STREAM_DEPTH, hardware.depth_width, hardware.depth_height, hardware.fps, OB_FORMAT_Y16);
        config->setFrameAggregateOutputMode(OB_FRAME_AGGREGATE_OUTPUT_ALL_TYPE_FRAME_REQUIRE);
#if 1
        // xxxjack this is really only for depth2color mode.
        config->setAlignMode(ALIGN_D2C_HW_MODE);
#endif
    } catch(ob::Error& e) {
        _log_error(std::string("enableVideoStream error: ") + e.what());
        return false;
    }
    return true;
}

void OrbbecPlaybackCamera::start_camera_streaming() {
    assert(camera_stopped);
    camera_stopped = false;
    _start_capture_thread();
    _start_processing_thread();
}
