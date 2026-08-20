#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <k4a/k4a.h>

#include "K4ABaseCapture.hpp"
#include "K4APlaybackCamera.hpp"

class K4APlaybackCapture : public K4ABaseCapture<k4a_playback_t, K4APlaybackCamera> {
    typedef k4a_playback_t Type_api_camera;
    typedef K4APlaybackCamera Type_our_camera;

public:
    static int count_devices() {
        return 0;
    }

    static K4APlaybackCapture* factory() {
        return new K4APlaybackCapture();
    }

    // methods
    bool seek(uint64_t timestamp) override;

protected:
    K4APlaybackCapture();
    virtual bool _capture_all_cameras(uint64_t& timestamp) override;
    virtual bool _apply_auto_config() override { return false; }

private:
    virtual bool _init_hardware_for_all_cameras() override final { return true; }
    virtual bool _check_cameras_connected() override final { return true;};
    virtual bool _create_cameras() override final;
    bool _open_recording_files(std::vector<Type_api_camera>& camera_handles); // Open the recordings

    // variables
    bool sync_inuse = false;
    int master_id = -1;
};
