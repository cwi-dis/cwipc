//
//  defs.h
//
//  Created by Fons Kuijk on 12-12-18.
//

#ifndef cwipc_realsense2_rs2config_h
#define cwipc_realsense2_rs2config_h

#include <cstdint>
#include <thread>

#include <pcl/common/transforms.h>
#include <pcl/common/common_headers.h>
#include <pcl/filters/voxel_grid.h>

//
// Definitions of types used across cwipc_realsense2, cwipc_codec and cwipc_util.
//
#include "cwipc_util/api_pcl.h"
#include "cwipc_util/internal/capturers.hpp"

struct RS2CameraProcessingParameters {
    int map_color_to_depth = 1;     // -1 means: no mapping at all.
    
    bool do_decimation = false;
    int decimation_magnitude = 1;             // int value between 2 and 8

    bool do_threshold = true;
    float threshold_min_distance = 0.15;         // float, near point for distance threshold
    float threshold_max_distance = 4.0;           // float, far point for distance threshold
    
    bool do_spatial = true;
    int spatial_magnitude = 2;           // int val between 1 and 5
    float spatial_smooth_alpha = 0.5;          // val between 0.25 and 1.0
    int spatial_smooth_delta = 20;               // int val between 1 and 50
    int spatial_holes_fill = 0;              // int val between 0 and 6
    
    bool do_temporal = false;
    float temporal_smooth_alpha = 0.4;          // val between 0 and 1
    int temporal_smooth_delta = 20;              // val between 1 and 100
    int temporal_persistency = 3;         // val between 0 and 8

    bool do_hole_filling = false;
    int hole_filling_mode = 0; 

};

struct RS2CameraConfig : CwipcBaseCameraConfig {
    int playback_inpoint_micros = 0; // for realsense_playback: initial seek for this camera.

    void _from_json(const json& json_data) override;
    void _to_json(json& json_data, bool for_recording=false) override;
};

struct RS2CameraHardwareConfig {
    // Limited documentation on the filters can be found at https://github.com/realsenseai/librealsense/blob/master/doc/post-processing-filters.md

    int color_width = 0;
    int color_height = 0;
    int depth_width = 0;
    int depth_height = 0;
    int fps = 0;
    int visual_preset = 0;                 // Visual preset mode for depth capturing. See emun rs2_rs400_visual_preset in rs_option.h
    int color_exposure = -1;              // Set exposure for color. -1 is auto.
    int color_gain = -1;                    // Set gain for color. -1 is auto.
    int depth_exposure = -1;              // Set exposure for depth. -1 is auto.
    int depth_gain = -1;                // Set gain for depth. -1 is auto.
    int whitebalance = -1;                // Set whitebalance for color camera. -1 is auto.
    int backlight_compensation = -1;      // Set backlight compesnation for color camera. -1 is don't change.
    int laser_power = -1;                // Laser power. -1 is don't change.
};

struct RS2CaptureProcessingConfig {
    // processing data
    bool greenscreen_removal = false;     // If true include greenscreen removal
    int depth_x_erosion = 0;              // How many valid depth pixels to remove in camera x direction
    int depth_y_erosion = 0;              // How many valid depth pixels to remove in camera y direction
    double height_min = 0.0;              // If height_min != height_max perform height filtering
    double height_max = 0.0;              // If height_min != height_max perform height filtering
    double radius_filter = 0.0;           // if > 0 apply filter around x=z=0 line.
};

struct RS2CaptureSyncConfig {
    std::string sync_master_serial = ""; // Serial number of master camera or "external"
    int sync_mode = 2; // For non-master cameras, when running synced: value for RS2_OPTION_INTER_CAM_SYNC_MODE
};

struct RS2CaptureMetadataConfig {
    bool want_rgb = false;
    bool want_depth = false;
    bool want_timestamps = false;
    bool want_camera_specs = false;
};

struct RS2CameraMetadataCameraSpecs {
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

struct RS2CaptureConfig : CwipcBaseCaptureConfig {
    // camera-independent processing (implemented in our code)
    RS2CaptureProcessingConfig processing;
    // Realsense-dependent processing (implemented in librealsense)
    RS2CameraProcessingParameters filtering;
    // Hardware parameters and processing (implemented in the camera hardware)
    RS2CameraHardwareConfig hardware;
    // Sync parameters
    RS2CaptureSyncConfig sync;

    // special features
    std::string record_to_directory = ""; // If non-empty all camera streams will be recorded to this directory.
    bool playback_realtime = false; // If true try to playback in realtime (possibly dropping a lot of frames)
    bool new_timestamps = false; // If true new timestamps are generated (otherwise original timestamps from capture time)
    bool playback_loop = false; // Set to true to loop the recording automatically.
    bool debug = false; // If true and if the relevant preprocessor symbol is defined print debug output to stdout.
    bool prefer_color_timing = true; // If we get a second frame with the same depth timestamp (but newer color frame) we skip the old one.

    // per camera data
    std::vector<RS2CameraConfig> all_camera_configs;

    std::string to_string(bool for_recording=false) override;
    bool from_string(const char* buffer, std::string typeWanted) override;
    bool from_file(const char* filename, std::string typeWanted) override;

    void _from_json(const json& json_data) override;
    void _to_json(json& json_data, bool for_recording=false) override;
};

#endif /* cwipc_realsense2_rs2config_h */
