#pragma once

#include "K4ABaseCamera.hpp"

class K4APlaybackCamera : public K4ABaseCamera<k4a_playback_t> {
    typedef k4a_playback_t Type_api_camera;
    const std::string CLASSNAME = "cwipc_kinect: K4APlaybackCamera";

private:
    K4APlaybackCamera(const K4APlaybackCamera&);  // Disable copy constructor
    K4APlaybackCamera& operator=(const K4APlaybackCamera&); // Disable assignment

public:
    K4APlaybackCamera(Type_api_camera _handle, K4ACaptureConfig& configuration, K4ACaptureMetadataConfig& metadata, int _camera_index);
    virtual ~K4APlaybackCamera() {}

    // virtual bool pre_start_all_cameras() override final { return true; }
    virtual bool start_camera() override final;
    // virtual void pre_stop_camera() override final {};
    virtual void stop_camera() override final;
    virtual uint64_t wait_for_captured_frameset(uint64_t earliest_timestamp) override final;
    bool seek(uint64_t timestamp);
    virtual bool eof() override final {
        //xxxjack to be implemented
        return false;
    }
protected:
    virtual bool _init_hardware_for_this_camera() override final {
        return true; // No hardware to initialize for the playback device
    }
    virtual void _start_capture_thread() override final {};
    virtual void _capture_thread_main() override final {};

private:
    bool _capture_next_valid_frame();
    bool _capture_frame_no_earlier_than_timestamp(uint64_t master_timestamp);

    uint64_t max_delay = 0;
    int capture_id = 0;
    k4a_record_configuration_t file_config;
    uint64_t current_frameset_timestamp = 0; //!< Needed so we can skip frames that are outdated.
};
