#pragma once

#include "K4ABaseCamera.hpp"

class K4ACamera : public K4ABaseCamera<k4a_device_t> {
    typedef k4a_device_t Type_api_camera;
    //const std::string CLASSNAME = "cwipc_kinect: K4ACamera";
    K4ACamera(const K4ACamera&);  // Disable copy constructor
    K4ACamera& operator=(const K4ACamera&); // Disable assignment

public:
    K4ACamera(Type_api_camera _handle, K4ACaptureConfig& configuration, K4ACaptureMetadataConfig& metadata, int _camera_index);
    virtual ~K4ACamera() {}

    // virtual bool pre_start_all_cameras() override final { return true; }
    virtual bool start_camera() override final;
    // virtual void pre_stop_camera() override final {}
    void stop_camera() override;
    virtual void get_camera_hardware_parameters(K4ACameraHardwareConfig& output) override final;
    virtual bool seek(uint64_t timestamp) override final { return false; }
    virtual bool eof() override final { return false; }
public:
    virtual uint64_t wait_for_captured_frameset(uint64_t minimum_timestamp) override final;
protected:
    virtual bool _init_hardware_for_this_camera() override final;
    virtual void _start_capture_thread() override final;
    virtual void _capture_thread_main() override final;

    k4a_record_t recorder = nullptr;
private:
    bool _init_config_for_this_camera(k4a_device_configuration_t& device_config);
};
