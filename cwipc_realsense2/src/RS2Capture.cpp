//
//  RS2Capture.cpp
//
//  Created by Fons Kuijk on 23-04-18
//

// Define to get (a little) debug prints
#undef CWIPC_DEBUG
#undef CWIPC_DEBUG_THREAD

// This is the dll source, so define external symbols as dllexport on windows.

#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_REALSENSE2_EXPORT __declspec(dllexport)
#define CWIPC_DLL_ENTRY __declspec(dllexport)
#endif

#include <chrono>

#include "RS2Config.hpp"
#include "RS2Capture.hpp"
#include "RS2Camera.hpp"

RS2Capture::RS2Capture()
: RS2BaseCapture("cwipc_realsense2::RS2Capture", "realsense")
{
}

RS2Capture* 
RS2Capture::factory() { 
    return new RS2Capture(); 
}

int 
RS2Capture::count_devices() {
    // Determine how many realsense cameras (not platform cameras like webcams) are connected
    const std::string platform_camera_name = "Platform Camera";
    rs2::context temp_context;
    rs2::device_list devs = temp_context.query_devices();
    int camera_count = 0;

    for (rs2::device dev : devs) {
        if (dev.get_info(RS2_CAMERA_INFO_NAME) != platform_camera_name) {
            camera_count++;
        }
    }

    return camera_count;
}

bool 
RS2Capture::seek(uint64_t timestamp) {
    return false;
}

bool RS2Capture::_check_cameras_connected() {
    for (RS2CameraConfig& cd : configuration.all_camera_configs) {
        if (!cd.connected && !cd.disabled) {
            _log_warning("Camera with serial " + cd.serial + " is not connected");
            return false;
        }
    }
    return true;
}


bool RS2Capture::_init_hardware_for_all_cameras() {
    rs2::device_list devs = capturer_context.query_devices();
    for (auto dev : devs) {
        auto allSensors = dev.query_sensors();
        auto depth_sensor = dev.first<rs2::depth_sensor>();
        auto color_sensor = dev.first<rs2::color_sensor>();
        // We set visual preset first, because Jack is not sure whether it
        // contains values for some of the next settings.
        if (depth_sensor.supports(RS2_OPTION_VISUAL_PRESET)) {
            depth_sensor.set_option(
                RS2_OPTION_VISUAL_PRESET,
                (float)configuration.hardware.visual_preset
            );
        }
        // Options for color sensor
        if (configuration.hardware.color_gain >= 0) {
            assert(color_sensor.supports(RS2_OPTION_GAIN));
            color_sensor.set_option(RS2_OPTION_GAIN, configuration.hardware.color_gain);
        }
        if (configuration.hardware.color_exposure >= 0) {
            assert(color_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE));
            color_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
            assert (color_sensor.supports(RS2_OPTION_EXPOSURE));
            color_sensor.set_option(RS2_OPTION_EXPOSURE, configuration.hardware.color_exposure);
        } else {
            if (color_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE)) {
                color_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
            }
        }
        if (configuration.hardware.whitebalance >= 0) {
            if (color_sensor.supports(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)) {
                color_sensor.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, 0);
            }

            if (color_sensor.supports(RS2_OPTION_WHITE_BALANCE)) {
                color_sensor.set_option(RS2_OPTION_WHITE_BALANCE, configuration.hardware.whitebalance);
            }
        } else {
            if (color_sensor.supports(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE)) {
                color_sensor.set_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE, 1);
            }
        }
        if (configuration.hardware.backlight_compensation >= 0) {
            if (color_sensor.supports(RS2_OPTION_BACKLIGHT_COMPENSATION)) {
                color_sensor.set_option(RS2_OPTION_BACKLIGHT_COMPENSATION, configuration.hardware.backlight_compensation);
            }
        }
        // Options for depth sensor
         if (configuration.hardware.depth_gain >= 0) {
            assert(depth_sensor.supports(RS2_OPTION_GAIN));
            depth_sensor.set_option(RS2_OPTION_GAIN, configuration.hardware.depth_gain);
        }
        if (configuration.hardware.depth_exposure >= 0) {
            assert(depth_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE));
            depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0);
            assert(depth_sensor.supports(RS2_OPTION_EXPOSURE));
            depth_sensor.set_option(RS2_OPTION_EXPOSURE, configuration.hardware.depth_exposure);
        } else {
            if (depth_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE)) {
                depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 1);
            }
        }
        if (depth_sensor.supports(RS2_OPTION_LASER_POWER) && configuration.hardware.laser_power >= 0) {
            depth_sensor.set_option(RS2_OPTION_LASER_POWER, configuration.hardware.laser_power);
        }
    }
    return true;
}

bool RS2Capture::_setup_inter_camera_sync() {
    std::string master_serial = configuration.sync.sync_master_serial;
    if (master_serial == "") {
        // Disable sync
        for(auto sensor : capturer_context.query_all_sensors()) {
           if (sensor.supports(RS2_OPTION_INTER_CAM_SYNC_MODE)) {
                sensor.set_option(RS2_OPTION_INTER_CAM_SYNC_MODE, 0);
           }
        }
        return true;
    }
    int nonmaster_sync_mode = configuration.sync.sync_mode;
    if (nonmaster_sync_mode == 0) {
        _log_warning("Sync_master_serial set, but no sync mode requested");
        return false;
    }
    bool use_external_sync = master_serial == "external";
    bool master_found = use_external_sync;

    rs2::device_list devs = capturer_context.query_devices();
    bool is_first_camera = true;
    for (auto dev : devs) {
        std::string serial(std::string(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER)));
        bool is_master = serial == master_serial;
        for (auto sensor : dev.query_sensors()) {
            if (sensor.supports(RS2_OPTION_INTER_CAM_SYNC_MODE)) {
                    
                if (is_master) {
                    sensor.set_option(RS2_OPTION_INTER_CAM_SYNC_MODE, 1);
                    master_found = true;
                    if (!is_first_camera) {
                        _log_warning("Camera " + master_serial + " is not the first camera found. This may influence sync");
                    }
                } else {
                    sensor.set_option(RS2_OPTION_INTER_CAM_SYNC_MODE, nonmaster_sync_mode);
                }
            }
        }
        is_first_camera = false;
    }
    if (!master_found) {
        _log_warning("Sync master camera with serial " + master_serial + " not found");
        return false;
    }
    return true;
}


bool RS2Capture::_apply_auto_config() {
    // Determine how many realsense cameras (not platform cameras like webcams) are connected
    const std::string platform_camera_name = "Platform Camera";
    rs2::device_list devs = capturer_context.query_devices();

    //
    // Enumerate over all connected cameras, create their default RS2CameraData structures
    // and set any hardware options (for example for sync).
    // We will create the actual RS2Camera objects later, after we have read the configuration file.
    //
    for (auto dev : devs) {
        if (dev.get_info(RS2_CAMERA_INFO_NAME) == platform_camera_name) {
            continue;
        }
        _log_debug("_apply_auto_config: Examining camera " + std::string(dev.get_info(RS2_CAMERA_INFO_NAME)));

        // Found a realsense camera. Create a default data entry for it.
        RS2CameraConfig cd;
        cd.type = "realsense";
        cd.serial = std::string(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        pcl::shared_ptr<Eigen::Affine3d> default_trafo(new Eigen::Affine3d());
        default_trafo->setIdentity();
        cd.trafo = default_trafo;
        cd.cameraposition = { 0, 0, 0 };
        configuration.all_camera_configs.push_back(cd);
    }
    if (configuration.all_camera_configs.size() == 0) {
        _log_error("no cameras found during auto-configuration");
        return false;
    }   
    return true;

}

bool RS2Capture::_create_cameras()
{
    rs2::device_list devs = capturer_context.query_devices();
    const std::string platform_camera_name = "Platform Camera";

    for (auto dev : devs) {
        if (dev.get_info(RS2_CAMERA_INFO_NAME) == platform_camera_name) {
            continue;
        }
        _log_debug("_create_cameras: Opening camera " + std::string(dev.get_info(RS2_CAMERA_INFO_NAME)));

        // Found a realsense camera. Create a default data entry for it.
        std::string serial(dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER));
        
        RS2CameraConfig* cd = get_camera_config(serial);

        if (cd == nullptr) {
            _log_warning("Camera with serial " + serial + " is connected but not in configuration");
            return false;
        }

        if (cd->type != "realsense") {
            _log_warning("Camera " + serial + " is type " + cd->type + " in stead of realsense");
            return false;
        }

        int camera_index = (int)cameras.size();

        if (cd->disabled) {
            // xxxnacho do we need to close the device, like the kinect case?
        } else {
            auto cam = new RS2Camera(capturer_context, configuration, metadata, camera_index);
            cameras.push_back(cam);
            cd->connected = true;
        }
    }

    return true;
}

RS2Capture::~RS2Capture() {
    _unload_cameras();
}


void RS2Capture::_initial_camera_synchronization() {

}