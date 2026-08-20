#include "OrbbecConfig.hpp"

#include <fstream>

void OrbbecCaptureConfig::_from_json(const json &json_data) {
    CwipcBaseCaptureConfig::_from_json(json_data);
    OrbbecCaptureConfig &config = *this;
    // version and type should already have been checked.

    json system_data = json_data.at("system");
    _CWIPC_CONFIG_JSON_GET(system_data, single_tile, config, single_tile);
    _CWIPC_CONFIG_JSON_GET(system_data, record_to_directory, config, record_to_directory);
    _CWIPC_CONFIG_JSON_GET(system_data, debug, config, debug);
    _CWIPC_CONFIG_JSON_GET(system_data, apiDebug, config, apiDebug);
    _CWIPC_CONFIG_JSON_GET(system_data, new_timestamps, config, new_timestamps);
    if (json_data.contains("sync")) {
        json sync_data = json_data.at("sync");
        _CWIPC_CONFIG_JSON_GET(sync_data, sync_master_serial, sync, sync_master_serial);
        _CWIPC_CONFIG_JSON_GET(sync_data, ignore_sync, sync, ignore_sync);
    }
    if (json_data.contains("hardware")) {
        json hardware_data = json_data.at("hardware");
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_width, hardware, color_width);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_height, hardware, color_height);
        _CWIPC_CONFIG_JSON_GET(hardware_data, depth_width, hardware, depth_width);
        _CWIPC_CONFIG_JSON_GET(hardware_data, depth_height, hardware, depth_height);
        _CWIPC_CONFIG_JSON_GET(hardware_data, fps, hardware, fps);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_exposure_time, hardware, color_exposure_time);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_whitebalance, hardware, color_whitebalance);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_backlight_compensation, hardware, color_backlight_compensation);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_brightness, hardware, color_brightness);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_contrast, hardware, color_contrast);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_saturation, hardware, color_saturation);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_sharpness, hardware, color_sharpness);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_gain, hardware, color_gain);
        _CWIPC_CONFIG_JSON_GET(hardware_data, color_powerline_frequency, hardware, color_powerline_frequency);
    }
    if (json_data.contains("processing")) {
        json processing_data = json_data.at("processing");
        _CWIPC_CONFIG_JSON_GET(processing_data, greenscreenremoval, processing, greenscreen_removal);
        _CWIPC_CONFIG_JSON_GET(processing_data, depth_x_erosion, processing, depth_x_erosion);
        _CWIPC_CONFIG_JSON_GET(processing_data, depth_y_erosion, processing, depth_y_erosion);
        _CWIPC_CONFIG_JSON_GET(processing_data, height_min, processing, height_min);
        _CWIPC_CONFIG_JSON_GET(processing_data, height_max, processing, height_max);
        _CWIPC_CONFIG_JSON_GET(processing_data, radius_filter, processing, radius_filter);
    }
    if (json_data.contains("filtering")) {
        json filtering_data = json_data.at("filtering");
        _CWIPC_CONFIG_JSON_GET(filtering_data, map_color_to_depth, filtering, map_color_to_depth);
        _CWIPC_CONFIG_JSON_GET(filtering_data, do_threshold, filtering, do_threshold);
        _CWIPC_CONFIG_JSON_GET(filtering_data, threshold_near, filtering, threshold_near);
        _CWIPC_CONFIG_JSON_GET(filtering_data, threshold_far, filtering, threshold_far);
    }
    json cameras = json_data.at("camera");
    int camera_index = 0;
    config.all_camera_configs.clear();

    for (json::iterator it = cameras.begin(); it != cameras.end(); it++) {
        json camera_config_json = *it;
        OrbbecCameraConfig camera_config;
        camera_config._from_json(camera_config_json);
        // xxxjack should check whether the camera with this serial already exists
        config.all_camera_configs.push_back(camera_config);
        camera_index++;
    }
    if (camera_index == 0) {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_orbbec", "No cameras in configuration");
    }
}

void OrbbecCaptureConfig::_to_json(json& json_data, bool for_recording) {
    CwipcBaseCaptureConfig::_to_json(json_data, for_recording);
    if (for_recording) {
        json_data["type"] = "orbbec_playback";
    }
    OrbbecCaptureConfig &config = *this;
    json cameras;
    int camera_index = 0;

    for (OrbbecCameraConfig camera_config : config.all_camera_configs) {
        json camera_config_json;
        camera_config._to_json(camera_config_json, for_recording);
        if (for_recording) {
          camera_config_json["type"] = "orbbec_playback";
        }
        cameras[camera_index] = camera_config_json;
        camera_index++;
    }

    json_data["camera"] = cameras;
    json system_data;
    _CWIPC_CONFIG_JSON_PUT(system_data, single_tile, config, single_tile);
    if (for_recording) {
        system_data["record_to_directory"] = "";
    } else {
        _CWIPC_CONFIG_JSON_PUT(system_data, record_to_directory, config, record_to_directory);
    }
    _CWIPC_CONFIG_JSON_PUT(system_data, new_timestamps, config, new_timestamps);
    _CWIPC_CONFIG_JSON_PUT(system_data, debug, config, debug);
    _CWIPC_CONFIG_JSON_PUT(system_data, apiDebug, config, apiDebug);
    json_data["system"] = system_data;

    json sync_data;
    _CWIPC_CONFIG_JSON_PUT(sync_data, sync_master_serial, sync, sync_master_serial);
    _CWIPC_CONFIG_JSON_PUT(sync_data, ignore_sync, sync, ignore_sync);
    json_data["sync"] = sync_data;

    json hardware_data;
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_width, hardware, color_width);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_height, hardware, color_height);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, depth_width, hardware, depth_width);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, depth_height, hardware, depth_height);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, fps, hardware, fps);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_exposure_time, hardware, color_exposure_time);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_whitebalance, hardware, color_whitebalance);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_backlight_compensation, hardware, color_backlight_compensation);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_brightness, hardware, color_brightness);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_contrast, hardware, color_contrast);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_saturation, hardware, color_saturation);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_gain, hardware, color_gain);
    _CWIPC_CONFIG_JSON_PUT(hardware_data, color_powerline_frequency, hardware, color_powerline_frequency);
    json_data["hardware"] = hardware_data;

    json processing_data;
    _CWIPC_CONFIG_JSON_PUT(processing_data, greenscreenremoval, processing, greenscreen_removal);
    _CWIPC_CONFIG_JSON_PUT(processing_data, depth_x_erosion, processing, depth_x_erosion);
    _CWIPC_CONFIG_JSON_PUT(processing_data, depth_y_erosion, processing, depth_y_erosion);
    _CWIPC_CONFIG_JSON_PUT(processing_data, height_min, processing, height_min);
    _CWIPC_CONFIG_JSON_PUT(processing_data, height_max, processing, height_max);
    _CWIPC_CONFIG_JSON_PUT(processing_data, radius_filter, processing, radius_filter);
    json_data["processing"] = processing_data;
    
    json filtering_data;
    _CWIPC_CONFIG_JSON_PUT(filtering_data, do_threshold, filtering, do_threshold);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, threshold_near, filtering, threshold_near);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, threshold_far, filtering, threshold_far);
    _CWIPC_CONFIG_JSON_PUT(filtering_data, map_color_to_depth, filtering, map_color_to_depth);
    json_data["filtering"] = filtering_data;

}

bool OrbbecCaptureConfig::from_file(const char *filename, std::string typeWanted) {
    json json_data;

    try {
        std::ifstream f(filename);

        if (!f.is_open()) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", std::string("CameraConfig ") + filename + " not found");
            return false;
        }

        json_data = json::parse(f);

        int version = 0;
        json_data.at("version").get_to(version);

        if (version != 5) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", std::string("CameraConfig ") + filename + " is not version 5");
            return false;
        }

        std::string type;
        json_data.at("type").get_to(type);

            if (type != typeWanted) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", std::string("CameraConfig ") + filename + "  type=" + type + " but expected " + typeWanted);
            return false;
        }

        _from_json(json_data);
    } catch (const std::exception &e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", std::string("CameraConfig ") + filename + ": exception " + e.what());
        return false;
    }

  return true;
}

bool OrbbecCaptureConfig::from_string(const char *jsonBuffer, std::string typeWanted) {
    json json_data;

    try {
        json_data = json::parse(jsonBuffer);

        int version = 0;
        json_data.at("version").get_to(version);

        if (version != 5) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", std::string("CameraConfig ") + "(inline buffer) " + "ignored, is not version 3");
            return false;
        }

        std::string type;
        json_data.at("type").get_to(type);

        if (type != typeWanted) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", std::string("CameraConfig ") + "(inline buffer) " + "ignored, is not " + typeWanted + " but " + type);
            return false;
        }

        _from_json(json_data);
    } catch (const std::exception &e) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", std::string("CameraConfig ") + "(inline buffer) " + ": exception " + e.what());
        return false;
    }

    return true;
}

std::string OrbbecCaptureConfig::to_string(bool for_recording) {
    json result;
    _to_json(result, for_recording);

    return result.dump(2);
}
