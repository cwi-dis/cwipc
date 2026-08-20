#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <k4a/k4a.h>

#include "K4AConfig.hpp"
#include "K4ABaseCapture.hpp"
#include "K4ACamera.hpp"


class K4ACapture : public K4ABaseCapture<k4a_device_t, K4ACamera> {
    typedef k4a_device_t Type_api_camera;
    typedef K4ACamera Type_our_camera;

public:
    static int count_devices();
    static K4ACapture* factory() { return new K4ACapture(); }
    // methods
    virtual ~K4ACapture() {}
    bool seek(uint64_t timestamp) override final;

protected:
    K4ACapture();
    virtual bool _capture_all_cameras(uint64_t& timestamp) override final;
    virtual bool _apply_auto_config() override final;

private:
    virtual bool _init_hardware_for_all_cameras() override final; // initialize hardware parameters from configuration
    virtual bool _check_cameras_connected() override final;
    virtual bool _create_cameras() override final;
};
