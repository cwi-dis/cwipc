#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_REALSENSE2_EXPORT __declspec(dllexport)
#endif
#include "RS2Config.hpp"

#include <fstream>

void RS2CameraConfig::_from_json(const json& json_data) {
    CwipcBaseCameraConfig::_from_json(json_data);
    RS2CameraConfig& config = *this;
    // version and type should already have been checked.

    _CWIPC_CONFIG_JSON_GET(json_data, playback_inpoint_micros, config, playback_inpoint_micros);
}

void RS2CameraConfig::_to_json(json& json_data, bool for_recording) {
    CwipcBaseCameraConfig::_to_json(json_data, for_recording);
    if (for_recording) {
        json_data["type"] = "realsense_playback";
    }
    RS2CameraConfig& config = *this;
    if (playback_inpoint_micros != 0) {
        _CWIPC_CONFIG_JSON_PUT(json_data, playback_inpoint_micros, config, playback_inpoint_micros);
    }
}

void RS2CaptureConfig::_from_json(const json& json_data) {
    CwipcBaseCaptureConfig::_from_json(json_data);
    RS2CaptureConfig& config = *this;
    // version and type should already have been checked.
    RS2CameraHardwareConfig& hardware(config.hardware);
    RS2CameraProcessingParameters& filtering(config.filtering);
    RS2CaptureProcessingConfig& processing(config.processing);
    RS2CaptureSyncConfig& sync(config.sync);

    json system_data = json_data.at("system");
    _CWIPC_CONFIG_JSON_GET(system_data, record_to_directory, config, record_to_directory);
    _CWIPC_CONFIG_JSON_GET(system_data, playback_realtime, config, playback_realtime);
    _CWIPC_CONFIG_JSON_GET(system_data, new_timestamps, config, new_timestamps);
    _CWIPC_CONFIG_JSON_GET(system_data, debug, config, debug);
    _CWIPC_CONFIG_JSON_GET(system_data, prefer_color_timing, config, prefer_color_timing);
    if (json_data.contains("sync"))
    {
        json sync_data = json_data.at("sync");
        _CWIPC_CONFIG_JSON_GET(sync_data, sync_master_serial, sync, sync_master_serial);
        _CWIPC_CONFIG_JSON_GET(sync_data, sync_mode, sync, sync_mode);
    }
    if (json_data.contains("hardware"))
    {
        json hardware_data = json_data.at("hardware");
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_width, hardware, color_width);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_height, hardware, color_height);
        _CWIPC_CONFIG_JSON_GET(hardware_data, fps, hardware, fps);
        _CWIPC_CONFIG_JSON_GET(hardware_data, depth_width, hardware, depth_width);
        _CWIPC_CONFIG_JSON_GET(hardware_data, depth_height, hardware, depth_height);
        _CWIPC_CONFIG_JSON_GET(hardware_data, fps, hardware, fps);
        _CWIPC_CONFIG_JSON_GET(hardware_data, visual_preset, hardware, visual_preset);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_exposure, hardware, color_exposure);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_gain, hardware, color_gain);
        _CWIPC_CONFIG_JSON_GET(hardware_data, depth_exposure, hardware, depth_exposure);
        _CWIPC_CONFIG_JSON_GET(hardware_data, depth_gain, hardware, depth_gain);
        _CWIPC_CONFIG_JSON_GET(hardware_data, whitebalance, hardware, whitebalance);
        _CWIPC_CONFIG_JSON_GET(hardware_data, backlight_compensation, hardware, backlight_compensation);
        _CWIPC_CONFIG_JSON_GET(hardware_data, laser_power, hardware, laser_power);
    }
    if (json_data.contains("processing"))
    {
        json processing_data = json_data.at("processing");
        _CWIPC_CONFIG_JSON_GET(processing_data, greenscreen_removal, processing, greenscreen_removal);
        _CWIPC_CONFIG_JSON_GET(processing_data, height_min, processing, height_min);
        _CWIPC_CONFIG_JSON_GET(processing_data, height_max, processing, height_max);
        _CWIPC_CONFIG_JSON_GET(processing_data, radius_filter, processing, radius_filter);
        _CWIPC_CONFIG_JSON_GET(processing_data, depth_x_erosion, processing, depth_x_erosion);
        _CWIPC_CONFIG_JSON_GET(processing_data, depth_y_erosion, processing, depth_y_erosion);
    }
    if (json_data.contains("filtering"))
    {   
        json filtering_data = json_data.at("filtering");
        _CWIPC_CONFIG_JSON_GET(filtering_data, map_color_to_depth, filtering, map_color_to_depth);
        _CWIPC_CONFIG_JSON_GET(filtering_data, do_decimation, filtering, do_decimation);
        _CWIPC_CONFIG_JSON_GET(filtering_data, decimation_magnitude, filtering, decimation_magnitude);
        _CWIPC_CONFIG_JSON_GET(filtering_data, do_threshold, filtering, do_threshold);
        _CWIPC_CONFIG_JSON_GET(filtering_data, threshold_min_distance, filtering, threshold_min_distance);
        _CWIPC_CONFIG_JSON_GET(filtering_data, threshold_max_distance, filtering, threshold_max_distance);
        _CWIPC_CONFIG_JSON_GET(filtering_data, do_spatial, filtering, do_spatial);
        _CWIPC_CONFIG_JSON_GET(filtering_data, spatial_magnitude, filtering, spatial_magnitude);
        _CWIPC_CONFIG_JSON_GET(filtering_data, spatial_smooth_alpha, filtering, spatial_smooth_alpha);
        _CWIPC_CONFIG_JSON_GET(filtering_data, spatial_smooth_delta, filtering, spatial_smooth_delta);
        _CWIPC_CONFIG_JSON_GET(filtering_data, spatial_holes_fill, filtering, spatial_holes_fill);
        _CWIPC_CONFIG_JSON_GET(filtering_data, do_temporal, filtering, do_temporal);
        _CWIPC_CONFIG_JSON_GET(filtering_data, temporal_smooth_alpha, filtering, temporal_smooth_alpha);
        _CWIPC_CONFIG_JSON_GET(filtering_data, temporal_smooth_delta, filtering, temporal_smooth_delta);
        _CWIPC_CONFIG_JSON_GET(filtering_data, temporal_persistency, filtering, temporal_persistency);
        _CWIPC_CONFIG_JSON_GET(filtering_data, do_hole_filling, filtering, do_hole_filling);
        _CWIPC_CONFIG_JSON_GET(filtering_data, hole_filling_mode, filtering, hole_filling_mode);
    }

    json cameras = json_data.at("camera");
    int camera_index = 0;
    config.all_camera_configs.clear();

    for(json::iterator it=cameras.begin(); it != cameras.end(); it++) {
        json camera_config_json = *it;
        RS2CameraConfig camera_config;
        camera_config._from_json(camera_config_json);
        config.all_camera_configs.push_back(camera_config);
        camera_index++;
    }
    if (camera_index == 0) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_realsense2", "No cameras in configuration");
    }
}

void RS2CaptureConfig::_to_json(json& json_data, bool for_recording) {
    CwipcBaseCaptureConfig::_to_json(json_data, for_recording);
    if (for_recording) {
        json_data["type"] = "realsense_playback";
    }
    RS2CaptureConfig& config = *this;
    json cameras;
    int camera_index = 0;

    for (RS2CameraConfig camera_config : config.all_camera_configs) {
        json camera_config_json;
        camera_config._to_json(camera_config_json, for_recording);

        cameras[camera_index] = camera_config_json;
        camera_index++;
    }

    json_data["camera"] = cameras;
    const RS2CaptureSyncConfig& sync(config.sync);
    json sync_data;
    _CWIPC_CONFIG_JSON_PUT(sync_data, sync_master_serial, sync, sync_master_serial);
    _CWIPC_CONFIG_JSON_PUT(sync_data, sync_mode, sync, sync_mode);
    json_data["sync"] = sync_data;
    
    const RS2CameraProcessingParameters& filtering(config.filtering);
    json filtering_data;
    _CWIPC_CONFIG_JSON_PUT(filtering_data, map_color_to_depth, filtering, map_color_to_depth);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, do_decimation, filtering, do_decimation);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, decimation_magnitude, filtering, decimation_magnitude);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, do_threshold, filtering, do_threshold);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, threshold_min_distance, filtering, threshold_min_distance);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, threshold_max_distance, filtering, threshold_max_distance);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, do_spatial, filtering, do_spatial);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, spatial_magnitude, filtering, spatial_magnitude);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, spatial_smooth_alpha, filtering, spatial_smooth_alpha);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, spatial_smooth_delta, filtering, spatial_smooth_delta);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, spatial_holes_fill, filtering, spatial_holes_fill);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, do_temporal, filtering, do_temporal);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, temporal_smooth_alpha, filtering, temporal_smooth_alpha);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, temporal_smooth_delta, filtering, temporal_smooth_delta);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, temporal_persistency, filtering, temporal_persistency);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, do_hole_filling, filtering, do_hole_filling);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, hole_filling_mode, filtering, hole_filling_mode);
    json_data["filtering"] = filtering_data;

    const RS2CaptureProcessingConfig& processing(config.processing);
    json processing_data;
    _CWIPC_CONFIG_JSON_PUT(processing_data, greenscreen_removal, processing, greenscreen_removal);
    _CWIPC_CONFIG_JSON_PUT(processing_data, height_min, processing, height_min);
    _CWIPC_CONFIG_JSON_PUT(processing_data, height_max, processing, height_max);
    _CWIPC_CONFIG_JSON_PUT(processing_data, radius_filter, processing, radius_filter);
    _CWIPC_CONFIG_JSON_PUT(processing_data, depth_x_erosion, processing, depth_x_erosion);
    _CWIPC_CONFIG_JSON_PUT(processing_data, depth_y_erosion, processing, depth_y_erosion);
    json_data["processing"] = processing_data;

    json system_data;
    if (for_recording) {
        system_data["record_to_directory"] = "";
    } else {
        _CWIPC_CONFIG_JSON_PUT(system_data, record_to_directory, config, record_to_directory);
    }
    _CWIPC_CONFIG_JSON_PUT(system_data, playback_realtime, config, playback_realtime);
    _CWIPC_CONFIG_JSON_PUT(system_data, new_timestamps, config, new_timestamps);
    _CWIPC_CONFIG_JSON_PUT(system_data, debug, config, debug);
    _CWIPC_CONFIG_JSON_PUT(system_data, prefer_color_timing, config, prefer_color_timing);
    json_data["system"] = system_data;
    
    const RS2CameraHardwareConfig& hardware(config.hardware);
    json hardware_data;
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_width, hardware, color_width);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_height, hardware, color_height);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, depth_width, hardware, depth_width);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, depth_height, hardware, depth_height);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, fps, hardware, fps);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, visual_preset, hardware, visual_preset);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_exposure, hardware, color_exposure);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_gain, hardware, color_gain);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, depth_exposure, hardware, depth_exposure);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, depth_gain, hardware, depth_gain);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, whitebalance, hardware, whitebalance);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, backlight_compensation, hardware, backlight_compensation);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, laser_power, hardware, laser_power);
    json_data["hardware"] = hardware_data;
}

bool RS2CaptureConfig::from_file(const char* filename, std::string typeWanted) {
    json json_data;

    try {
        std::ifstream f(filename);

        if (!f.is_open()) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", std::string("CameraConfig ") + filename + " not found");
            return false;
        }
        json_data = json::parse(f);

        int version = 0;
        json_data.at("version").get_to(version);

        if (version != 5 && version != 4 && version != 3) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", std::string("CameraConfig ") + filename + " ignored, is not version 5, 4 or 3");
            return false;
        }

        std::string type;
        json_data.at("type").get_to(type);

        if (type != typeWanted) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", std::string("CameraConfig ") + filename + " ignored, is not " + typeWanted + " but " + type);
            return false;
        }

        _from_json(json_data);
    } catch (const std::exception& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", std::string("CameraConfig ") + filename + ": exception " + e.what() );
        return false;
    }

    return true;
}

bool RS2CaptureConfig::from_string(const char* jsonBuffer, std::string typeWanted) {
    json json_data;

    try {
        json_data = json::parse(jsonBuffer);

        int version = 0;
        json_data.at("version").get_to(version);

        if (version != 3) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", std::string("CameraConfig ") + "(inline buffer) " + " is not version 3");
            return false;
        }

        std::string type;
        json_data.at("type").get_to(type);

        if (type != typeWanted) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", std::string("CameraConfig ") + "(inline buffer) " + " type=" + type + " but expected " + typeWanted);
            return false;
        }

        _from_json(json_data);
    } catch (const std::exception& e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", std::string("CameraConfig ") + "(inline buffer) " + ": exception " + e.what() );
        return false;
    }

    return true;
}

std::string RS2CaptureConfig::to_string(bool for_recording) {
    json result;
    _to_json(result, for_recording);

    return result.dump(2);
}
