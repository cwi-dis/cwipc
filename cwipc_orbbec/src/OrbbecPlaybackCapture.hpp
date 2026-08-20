#pragma once

#include "libobsensor/hpp/Context.hpp"

#include "OrbbecBaseCapture.hpp"
#include "OrbbecPlaybackCamera.hpp"

class OrbbecPlaybackCapture : public OrbbecBaseCapture<std::shared_ptr<ob::Device>, OrbbecPlaybackCamera> {
    typedef std::shared_ptr<ob::Device> Type_api_camera;
    typedef OrbbecPlaybackCamera Type_our_camera;
public:
    virtual ~OrbbecPlaybackCapture() {
    }
    static int countDevices() {
        return 0;
    }

    static OrbbecPlaybackCapture* factory() {
        return new OrbbecPlaybackCapture();
    }


    bool seek(uint64_t timestamp) override;
protected:
    OrbbecPlaybackCapture();
    virtual bool _create_cameras() override final;
    virtual bool _init_hardware_for_all_cameras() override final { return true; };
    virtual bool _check_cameras_connected() override final { return true;};
    virtual bool _apply_config(const char* configFilename) override final;
    virtual bool _apply_auto_config() override final { return false; };

    virtual void _initial_camera_synchronization() override final;
private:
    std::string base_directory = "";
    uint64_t earliest_recording_timestamp_seen = 0;
};
