#include "OrbbecPlaybackCapture.hpp"

OrbbecPlaybackCapture::OrbbecPlaybackCapture()
:   OrbbecBaseCapture<Type_api_camera,Type_our_camera>("cwipc_orbbec::OrbbecPlaybackCapture", "orbbec_playback")
{
}

bool OrbbecPlaybackCapture::_apply_config(const char* configFilename) {
    bool ok = OrbbecBaseCapture::_apply_config(configFilename);
    if (!ok) {
        return ok;
    }
    if ( configFilename != NULL && configFilename[0] != '{' && (
        strchr(configFilename, '/') != NULL
#ifdef WIN32
        || strchr(configFilename, '\\') != NULL
#endif
        )) {
        // It appears the configFilename is a pathname. Get the directory component.
        std::string dirName(configFilename);
        int slashPos = dirName.find_last_of("/\\");
        base_directory = dirName.substr(0, slashPos+1);
    }
    return true;
}

bool OrbbecPlaybackCapture::_create_cameras() {
    for (OrbbecCameraConfig& cd : configuration.all_camera_configs) {
        // Ensure we have a serial
        if (cd.serial == "") {
            cd.serial = cd.filename;
        }
        if (cd.filename == "" && cd.serial == "") {
            _log_error("Camera " + cd.serial + " has no filename nor serial");
            return false;
        }
        if (cd.type != "orbbec_playback") {
            _log_warning("Camera " + cd.serial + " is type " + cd.type + " in stead of orbbec_playback");
            return false;
        }

        int camera_index = (int)cameras.size();

        if (cd.disabled) {
            // xxxnacho do we need to close the device, like the kinect case?
        }
        else {
            std::string recording_filename = cd.filename;
            if (recording_filename == "") recording_filename = cd.serial + ".bag";
            if (base_directory != "") {
                recording_filename = base_directory + recording_filename;
            }
            auto cam = new OrbbecPlaybackCamera(nullptr, configuration, metadata, camera_index, recording_filename);
            cameras.push_back(cam);
            cd.connected = true;
        }

    }
    return true;

}

void OrbbecPlaybackCapture::_initial_camera_synchronization() {
    // xxxjack implement me
}


bool OrbbecPlaybackCapture::seek(uint64_t timestamp) {
    return false;
}
