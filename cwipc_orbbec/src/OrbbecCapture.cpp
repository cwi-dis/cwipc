#include "OrbbecCapture.hpp"

OrbbecCapture::OrbbecCapture()
:   OrbbecBaseCapture<Type_api_camera,Type_our_camera>("cwipc_orbbec::OrbbecCapture", "orbbec")
{
}

int OrbbecCapture::countDevices() {
    ob::Context context;
    auto deviceList = context.queryDeviceList();

    return deviceList->deviceCount();
}

bool OrbbecCapture::_apply_auto_config() {
    ob::Context context;
    auto deviceList = context.queryDeviceList();
    int nDevice = deviceList->getCount();
    for (int i = 0; i < nDevice; i++) {
        OrbbecCameraConfig config;

        config.type = "orbbec";
        config.serial = deviceList->serialNumber(i);

        pcl::shared_ptr<Eigen::Affine3d> default_trafo(new Eigen::Affine3d());
        default_trafo->setIdentity();

        config.trafo = default_trafo;
        config.cameraposition = { 0, 0, 0 };

        configuration.all_camera_configs.push_back(config);
    }
    if (configuration.all_camera_configs.size() == 0) {
        _log_error("no cameras found during auto-configuration");
        return false;
    }
    return true;
}


bool OrbbecCapture::_init_hardware_for_all_cameras() {
    return true;
}

bool OrbbecCapture::_create_cameras() {
    ob::Context context;
#ifdef CWIPC_ORBBEC_LOGGING
    context.setLoggerToConsole(OB_LOG_SEVERITY_DEBUG); // xxxjack
#endif
    auto deviceList = context.queryDeviceList();

    for (int i = 0; i < deviceList->deviceCount(); i++) {
        Type_api_camera handle = deviceList->getDevice(i);
        const char* serialNum = deviceList->serialNumber(i);
        OrbbecCameraConfig* cd = get_camera_config(serialNum);

        if (cd == nullptr) {
            _log_error(std::string("Camera with serial ") + serialNum + " is connected but not in configuration");
            return false;
        }

        if (cd->type != "orbbec") {
            _log_error(std::string("Camera ") + serialNum + " is type " + cd->type + " in stead of orbbec");
            return false;
        }
        if (cd->disabled) {
            // xxxjack do we need to close it?
        } else {
            int camera_index = cameras.size();
            auto cam = _create_single_camera(handle, configuration, metadata, camera_index);
            cameras.push_back(cam);
            cd->connected = true;
        }
    }
    return true;
}


bool OrbbecCapture::_check_cameras_connected() {
    for (auto& cd : configuration.all_camera_configs) {
        if (!cd.connected && !cd.disabled) {
            _log_warning("Camera with serial " + cd.serial + " is not connected");
            return false;
        }
    }
    return true;
}

bool OrbbecCapture::seek(uint64_t timestamp) {
    return false;
}
