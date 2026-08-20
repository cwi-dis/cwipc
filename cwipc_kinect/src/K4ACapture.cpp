// Define to get (a little) debug prints
#undef CWIPC_DEBUG
#undef CWIPC_DEBUG_THREAD

#include <chrono>

#include "K4ACapture.hpp"

K4ACapture::K4ACapture()
: K4ABaseCapture("K4ACapture", "kinect")
{}

int K4ACapture::count_devices() {
    return k4a_device_get_installed_count();
}


bool K4ACapture::_apply_auto_config() {
    bool any_failure = false;

    // Enumerate over all attached cameras and create a default configuration
    for (uint32_t i = 0; i < k4a_device_get_installed_count(); i++) {
        Type_api_camera handle;

        if (k4a_device_open(i, &handle) != K4A_RESULT_SUCCEEDED) {
            _log_error("k4a_device_open failed");
            any_failure = true;
            continue;
        }

        K4ACameraConfig cd;
        cd.type = "kinect";
        char serial_buf[64];
        size_t serial_buf_size = sizeof(serial_buf) / sizeof(serial_buf[0]);

        if (k4a_device_get_serialnum(handle, serial_buf, &serial_buf_size) != K4A_BUFFER_RESULT_SUCCEEDED) {
            _log_error("k4a_device_get_serialnum failed");
            any_failure = true;

            continue;
        }

        cd.serial = std::string(serial_buf);
        pcl::shared_ptr<Eigen::Affine3d> default_trafo(new Eigen::Affine3d());
        default_trafo->setIdentity();
        cd.trafo = default_trafo;
        cd.cameraposition = { 0, 0, 0 };
        configuration.all_camera_configs.push_back(cd);
        k4a_device_close(handle);
    }

    if (any_failure) {
        _unload_cameras();
        return false;
    }
    if (configuration.all_camera_configs.size() == 0) {
        _log_error("no cameras found during auto-configuration");
        return false;
    }
    return true;
}

bool K4ACapture::_create_cameras() {
    for (uint32_t i = 0; i < k4a_device_get_installed_count(); i++) {
        Type_api_camera handle;

        if (k4a_device_open(i, &handle) != K4A_RESULT_SUCCEEDED) {
            _log_error("k4a_device_open failed");
            return false;
        }

        char serial_buf[64];
        size_t serial_buf_size = sizeof(serial_buf) / sizeof(serial_buf[0]);

        if (k4a_device_get_serialnum(handle, serial_buf, &serial_buf_size) != K4A_BUFFER_RESULT_SUCCEEDED) {
            _log_error("k4a_device_get_serialnum failed");
            return false;
        }

        std::string serial(serial_buf);
        K4ACameraConfig* cd = get_camera_config(serial);

        if (cd == nullptr) {
            _log_error("Camera with serial " + serial + " is connected but not in configuration");
            return false;
        }
        if (cd->type != "kinect") {
            _log_error("configuration: camera with serial " + cd->serial + " is type " + cd->type + " instead of kinect");
            return false;
        }
        if (cd->disabled) {
            k4a_device_close(handle);
        } else {
            int camera_index = cameras.size();
            auto cam = _create_single_camera(handle, configuration, metadata, camera_index);
            cameras.push_back(cam);
            cd->connected = true;
        }

    }
    return true;
}

bool K4ACapture::_check_cameras_connected() {
    for (auto& cd : configuration.all_camera_configs) {
        if (!cd.connected && !cd.disabled) {
            _log_warning("Camera with serial " + cd.serial + " is not connected");
            return false;
        }
    }
    return true;
}

bool K4ACapture::_init_hardware_for_all_cameras() {

    return true;
}

bool K4ACapture::_capture_all_cameras(uint64_t& timestamp) {
        uint64_t first_timestamp = 0;
        for(auto cam : cameras) {
            uint64_t this_cam_timestamp = cam->wait_for_captured_frameset(first_timestamp);
            if (cam->end_of_stream_reached) return false;
            if (this_cam_timestamp == 0) {
                _log_warning("no frameset captured from camera " + cam->serial);
                return false;
            }
            if (first_timestamp == 0) {
                first_timestamp = this_cam_timestamp;
            }
        }

        // And get the best timestamp
        if (configuration.new_timestamps) {
            timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        } else {
            timestamp = first_timestamp;
        }
        return true;
}

bool K4ACapture::seek(uint64_t timestamp) {
    return false;
}
