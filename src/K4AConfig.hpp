//
//  defs.h
//
//  Created by Fons Kuijk on 12-12-18.
//
#pragma once

#include <cstdint>
#include <thread>

#include <pcl/common/transforms.h>
#include <pcl/common/common_headers.h>
#include <pcl/filters/voxel_grid.h>

//
// Definitions of types used across cwipc_kinect, cwipc_codec and cwipc_util.
//
#include "cwipc_util/api_pcl.h"
#include "cwipc_util/internal/capturers.hpp"


struct K4ACameraProcessingParameters {
    bool do_threshold = true;
    double threshold_near = 0.15;         // float, near point for distance threshold
    double threshold_far = 6.0;           // float, far point for distance threshold
    bool map_color_to_depth = false; // default DEPTH_TO_COLOR
};

struct K4ACameraConfig : CwipcBaseCameraConfig {
    // No extra parameters yet.
};


struct K4ACameraHardwareConfig {
    int color_height = 720;         // width of color frame (720, 1080 and various other values allowed, see kinect docs)
    int depth_height = 576;         // width of depth frame (288, 576, 512 and 1024 allowed)
    int fps = 30;                   // capture fps (5, 15 and 30 allowed)
    int32_t color_exposure_time = -1;     // default for manual: 40000;
    int32_t color_whitebalance = -1;   // default for manual: 3160; range(2500-12500)
    int32_t color_backlight_compensation = 0;     // default for manual: 0;
    int32_t color_brightness = 128;        // default for manual: 128;
    int32_t color_contrast = 5;          // default for manual: 5;
    int32_t color_saturation = 32;        // default for manual: 32;
    int32_t color_sharpness = 2;         // default for manual: 2;
    int32_t color_gain = 0;              // default for manual: 100;
    int32_t color_powerline_frequency = 2;     // default for manual: 2;
    bool color_mjpeg = false;       // Capture cameras in MJPEG in stead of BGRA.
};

struct K4ACaptureProcessingConfig {
    bool greenscreen_removal = false;   // If true include greenscreen removal
    int depth_x_erosion = 0;        // How many valid depth pixels to remove in camera x direction
    int depth_y_erosion = 0;        // How many valid depth pixels to remove in camera y direction
    double height_min = 0.0;        // If height_min != height_max perform height filtering
    double height_max = 0.0;        // If height_min != height_max perform height filtering
    double radius_filter = 0.0;     // If radius_filter > 0 we will remove all points further than radius_filter from the (0,1,0) axis
};

struct K4ACaptureSyncConfig {
    std::string sync_master_serial = "";  // If empty run without sync. If non-empty this camera is the sync master
    bool ignore_sync = false;  // If true dont look at camera master/sub mode in files.
};

struct K4ASkeletonConfig {
    int bt_sensor_orientation = -1; // Override k4abt sensor_orientation (if >= 0)
    int bt_processing_mode = -1;  // Override k4abt processing_mode (if >= 0)
    std::string bt_model_path = "";     // Override k4abt model path
};

struct K4ACaptureMetadataConfig {
    bool want_rgb = false;
    bool want_depth = false;
    bool want_skeleton = false;
    bool want_camera_specs = false;
};

struct K4ACameraMetadataCameraSpecs {
    // Focal length in pixels
    float focal_length_x;
    float focal_length_y;
    // Principal point (optical center) in pixels
    float principal_point_x;
    float principal_point_y;
    // Color image size
    unsigned int color_image_width;
    unsigned int color_image_height;
    // Default near and far plane
    float near_plane;
    float far_plane;
};

struct K4ACaptureConfig : CwipcBaseCaptureConfig {
    K4ACaptureProcessingConfig processing;
    K4ACameraProcessingParameters filtering;
    K4ACameraHardwareConfig hardware;
    K4ACaptureSyncConfig sync;
    K4ASkeletonConfig skeleton;
    int single_tile = -1;       // if singletile >=0 all the points will be the specified integer

    std::string record_to_directory = ""; // If non-empty all camera streams will be recorded to this directory.
    bool new_timestamps = false; // If true new timestamps are generated (otherwise original timestamps from capture time)
    bool debug = false; // If true and if the relevant preprocessor symbol is defined print debug output to stdout.
    bool apiDebug = false;
    // We could probably also allow overriding GPU id and model path, but no need for now.
    // per camera data
    std::vector<K4ACameraConfig> all_camera_configs;

    std::string to_string(bool for_recording=false) override;
    bool from_string(const char* buffer, std::string typeWanted) override;
    bool from_file(const char* filename, std::string typeWanted) override;

    void _from_json(const json& json_data) override;
    void _from_json_v4(const json& json_data);
    void _to_json(json& json_data, bool for_recording=false) override;
};
