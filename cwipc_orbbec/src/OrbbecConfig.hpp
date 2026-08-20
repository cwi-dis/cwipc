#pragma once
#define CWIPC_DEBUG
#define CWIPC_DEBUG_THREAD
#include <thread>
#include "libobsensor/hpp/Device.hpp"

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/internal/capturers.hpp"

struct OrbbecCameraProcessingParameters {
    bool do_threshold = true;
    double threshold_near = 0.15;         // float, near point for distance threshold
    double threshold_far = 6.0;           // float, far point for distance threshold
    bool map_color_to_depth = false; // default DEPTH_TO_COLOR
};

struct OrbbecCameraConfig : public CwipcBaseCameraConfig {
};

struct OrbbecCameraHardwareConfig {
    int color_width = 1280;                     // width of color frame (720, 1080 and various other values allowed, see kinect docs)
    int color_height = 720;                     // height of color frame (720, 1080 and various other values allowed, see kinect docs)
    int depth_width = 640;                // width of depth frame (288, 576, 512 and 1024 allowed)
    int depth_height = 576;                // height of depth frame (288, 576, 512 and 1024 allowed)
    int fps = 15;                         // capture fps (5, 15 and 30 allowed)
    int32_t color_exposure_time = -200;     // default for manual: 200, auto;
    int32_t color_whitebalance = -6500;   // default for manual: 6500, auto;
    int32_t color_backlight_compensation = 0;     // Not implemented?
    int32_t color_brightness = 0;        // default for manual: 0;
    int32_t color_contrast = 32;          // default for manual: 32;
    int32_t color_saturation = 54;        // default for manual: 54;
    int32_t color_sharpness = 6;         // default for manual: 6;
    int32_t color_gain = 60;              // default for manual: 60;
    int32_t color_powerline_frequency = 2;     // default for manual: 2;

};

struct OrbbecCaptureProcessingConfig {
    bool greenscreen_removal = false;   // If true include greenscreen removal
    int depth_x_erosion = 0;        // How many valid depth pixels to remove in camera x direction
    int depth_y_erosion = 0;        // How many valid depth pixels to remove in camera y direction
    double height_min = 0.0;        // If height_min != height_max perform height filtering
    double height_max = 0.0;        // If height_min != height_max perform height filtering
    double radius_filter = 0.0;     // If radius_filter > 0 we will remove all points further than radius_filter from the (0,1,0) axis
};

struct OrbbecCaptureSyncConfig {
    std::string sync_master_serial = "";  // If empty run without sync. If non-empty this camera is the sync master
    bool ignore_sync = false;  // If true dont look at camera master/sub mode in files.

};

struct OrbbecCaptureMetadataConfig {
    bool want_rgb = false;
    bool want_depth = false;
};
struct OrbbecCaptureConfig : public CwipcBaseCaptureConfig {
    OrbbecCaptureProcessingConfig processing;
    OrbbecCameraProcessingParameters filtering;
    OrbbecCameraHardwareConfig hardware;
    OrbbecCaptureSyncConfig sync;
    int single_tile = -1;       // if singletile >=0 all the points will be the specified integer

    OrbbecCameraProcessingParameters camera_processing;
    std::string record_to_directory = ""; // If non-empty all camera streams will be recorded to this directory.
    bool new_timestamps = false; // If true new timestamps are generated (otherwise original timestamps from capture time)
    bool debug = false;
    bool apiDebug = false;
    // We could probably also allow overriding GPU id and model path, but no need for now.
    // per camera data
    std::vector<OrbbecCameraConfig> all_camera_configs;

    std::string to_string(bool for_recording=false) override;
    bool from_string(const char* buffer, std::string typeWanted) override;
    bool from_file(const char* filename, std::string typeWanted) override;

    void _from_json(const json& json_data) override;
    void _to_json(json& json_data, bool for_recording=false) override;
};
