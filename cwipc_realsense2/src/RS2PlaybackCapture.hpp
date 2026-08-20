#ifndef cwipc_realsense_RS2PlaybackCapture_hpp
#define cwipc_realsense_RS2PlaybackCapture_hpp
#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

#include <librealsense2/rs.hpp>

#include "RS2Config.hpp"
#include "RS2BaseCapture.hpp"
#include "RS2PlaybackCamera.hpp"

class RS2PlaybackCapture : public RS2BaseCapture<class RS2PlaybackCamera, class RS2CameraConfig> {
public:
    virtual ~RS2PlaybackCapture();
    static int count_devices();
    static RS2PlaybackCapture* factory();
    bool seek(uint64_t timestamp) override; 
protected:
    RS2PlaybackCapture();   
    virtual bool _create_cameras() override final;
    virtual bool _setup_inter_camera_sync() override final { return true; };
    virtual bool _init_hardware_for_all_cameras() override final { return true; };
    virtual bool _check_cameras_connected() override final { return true;};
    virtual bool _apply_config(const char* configFilename) override final;
    virtual bool _apply_auto_config() override final { return false; };

    virtual void _initial_camera_synchronization() override final;
private:
    std::string base_directory = "";
    uint64_t earliest_recording_timestamp_seen = 0;
};
#endif // cwipc_realsense_RS2PlaybackCapture_hpp
