#ifndef cwipc_realsense_RS2Capture_hpp
#define cwipc_realsense_RS2Capture_hpp
#pragma once

#include "RS2BaseCapture.hpp"
#include "RS2Camera.hpp"
#include "RS2Config.hpp"

class RS2Capture : public RS2BaseCapture<class RS2Camera, class RS2CameraConfig> {
public:
    using RS2BaseCapture<class RS2Camera, class RS2CameraConfig>::RS2BaseCapture;
    virtual ~RS2Capture();
    static int count_devices();
    static RS2Capture* factory();

    virtual bool seek(uint64_t timestamp) override; 
protected:
    RS2Capture();
    virtual bool _create_cameras() override final;
    virtual bool _setup_inter_camera_sync() override final;
    virtual bool _init_hardware_for_all_cameras() override final;
    virtual bool _check_cameras_connected() override final;
    virtual bool _apply_auto_config() override final;
    virtual void _initial_camera_synchronization() override final;
};
#endif // cwipc_realsense_RS2Capture_hpp
