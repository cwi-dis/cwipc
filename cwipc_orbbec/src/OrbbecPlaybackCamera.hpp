#pragma once

#include "libobsensor/hpp/Device.hpp"
#include "libobsensor/hpp/RecordPlayback.hpp"
#include "OrbbecBaseCamera.hpp"

class OrbbecPlaybackCamera : public OrbbecBaseCamera<std::shared_ptr<ob::PlaybackDevice>> {
    typedef std::shared_ptr<ob::PlaybackDevice> Type_api_camera;

    OrbbecPlaybackCamera(const OrbbecPlaybackCamera&);
    OrbbecPlaybackCamera& operator=(const OrbbecPlaybackCamera&);

public:
    OrbbecPlaybackCamera(Type_api_camera camera, OrbbecCaptureConfig& config, OrbbecCaptureMetadataConfig& metadata, int cameraIndex, std::string filename);

    virtual ~OrbbecPlaybackCamera() {
        playback_device = nullptr;
    }

    // pre_start_all_cameras() defined in base class
    bool start_camera() override final;
    virtual void start_camera_streaming() override final;
    // pre_stop_camera() defined in base class
    // stop_camera() defined in base class.
    virtual bool seek(uint64_t timestamp) override final;
    virtual bool eof() override final;
    virtual void pause();
    virtual void resume();

protected:
    bool _init_pipeline_for_this_camera(std::shared_ptr<ob::Config> config);
    virtual bool _init_hardware_for_this_camera() override final { return true; }
    void _post_start_this_camera();
    virtual void _start_capture_thread() override final {}
    virtual void _capture_thread_main() override final {}
    void _stopped_callback();
private:
    std::string playback_filename;
    std::shared_ptr<ob::PlaybackDevice> playback_device;    //< Same object as camera_device
    bool playback_eof = false;
};
