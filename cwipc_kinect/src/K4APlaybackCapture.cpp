// Define to get (a little) debug prints
#define CWIPC_DEBUG
#undef CWIPC_DEBUG_THREAD

#include <chrono>

#include "K4APlaybackCapture.hpp"

// Static variable used to print a warning message when we re-create an K4APlaybackCapture
// if there is another one open.
static int numberOfCapturersActive = 0;

K4APlaybackCapture::K4APlaybackCapture()
: K4ABaseCapture("K4APlaybackCapture", "kinect_playback")
{
}


bool K4APlaybackCapture::_open_recording_files(std::vector<Type_api_camera>& camera_handles) {
    int n_files = camera_handles.size();
    if (n_files == 0) {
        // no camera connected, so we'll return nothing
        return false;
    }
    bool master_found = false;
    k4a_result_t result;

    // Open each recording file and validate they were recorded in master/subordinate mode.
    for (size_t i = 0; i < n_files; i++) {
        

        std::string camerafile(configuration.all_camera_configs[i].filename);
        if (camerafile == "") {
            camerafile = configuration.all_camera_configs[i].serial + ".mkv";
        }

        if (camerafile.substr(0, 1) != "/") {
            // Relative path (so don''t use windows drive numbers;-)
            size_t lastSlashPos = configurationCurrentFilename.find_last_of("/\\");

            if (lastSlashPos != std::string::npos) {
                camerafile = configurationCurrentFilename.substr(0, lastSlashPos + 1) + camerafile;
            }
        }

        result = k4a_playback_open(camerafile.c_str(), &camera_handles[i]);

        if (result != K4A_RESULT_SUCCEEDED) {
            _log_error("Failed to open recording file: " + camerafile);
            return false;
        }

        k4a_record_configuration_t file_config;
        result = k4a_playback_get_record_configuration(camera_handles[i], &file_config);

        if (result != K4A_RESULT_SUCCEEDED) {
            _log_error("Failed to get recording file configuration: " + camerafile);
            return false;
        }
        if (n_files > 1 && !configuration.sync.ignore_sync) {
            if (file_config.wired_sync_mode == K4A_WIRED_SYNC_MODE_MASTER) {
                _log_trace("Opened master recording file: " + camerafile);

                if (master_found) {
                    _log_error("Multiple master recording files");
                    result = K4A_RESULT_FAILED;

                    return false;
                }
                else {
                    master_found = true;
                    master_id = i;
                }
            }
            else if (file_config.wired_sync_mode == K4A_WIRED_SYNC_MODE_SUBORDINATE) {
                _log_trace("Opened subordinate recording file: " + camerafile);
            }
            else {
                _log_error("Recording file was not recorded in master/sub mode: " + camerafile);
                result = K4A_RESULT_FAILED;

                return false;
            }
        }
        char serial_buf[100];
        size_t serial_buf_size = 100;
        k4a_buffer_result_t result2 = k4a_playback_get_tag(camera_handles[i], "K4A_DEVICE_SERIAL_NUMBER", serial_buf, &serial_buf_size);
        if (result2 == K4A_BUFFER_RESULT_SUCCEEDED) {
            configuration.all_camera_configs[i].serial = std::string(serial_buf);
        }
        else
        {
            _log_error("Failed to get device serial number from recording file: " + camerafile);
            return false;
        }

        //initialize cameradata attributes:
        configuration.all_camera_configs[i].cameraposition = { 0, 0, 0 };
    }
    if (configuration.sync.sync_master_serial == "" && master_id >= 0) {
        configuration.sync.sync_master_serial = configuration.all_camera_configs[master_id].serial;
    }
    // xxxjack we should chack that configuration.sync_master_serial matches master_id...

    if (master_id != -1 && configuration.sync.sync_master_serial != "") {
        sync_inuse = true;
    }

    return true;
}

bool K4APlaybackCapture::_create_cameras() {
    auto camera_config_count = configuration.all_camera_configs.size();
    std::vector<Type_api_camera> camera_handles(camera_config_count, nullptr);
    if (!_open_recording_files(camera_handles)) {
        _unload_cameras();
        return false;
    }
    for (uint32_t i = 0; i < camera_handles.size(); i++) {
        assert (camera_handles[i] != nullptr);


        // Found a kinect camera. Create a default data entry for it.
        K4ACameraConfig& cd = configuration.all_camera_configs[i];
        if (configuration.debug) _log_debug("opening camera " + cd.serial);
        if (cd.type == "kinect_offline") {
            _log_warning("camera with serial " + cd.serial + " has deprecated type 'kinect_offline', changing to 'kinect_playback'");
            cd.type = "kinect_playback";
        }
        if (cd.type != "kinect_playback") {
            _log_error("camera with serial " + cd.serial + " has wrong type " + cd.type);
        }

        int camera_index = cameras.size();

        auto cam = _create_single_camera(camera_handles[i], configuration, metadata, camera_index);
        cameras.push_back(cam);
    }
    return true;
}

bool K4APlaybackCapture::_capture_all_cameras(uint64_t& timestamp) {

    //
    //f irst capture master frame (it is the referrence to sync).
    // For the master we simply get the next frame available (indicated by timestamp==0)
    //
   for (auto cam : cameras) { //MASTER
        if (cam->is_sync_master()) {
            timestamp = cam->wait_for_captured_frameset(timestamp);
            if (timestamp == 0) {
                // For the master camera this is end-of-file
                // _log_warning("failed to capture frameset for master " + cam->serial);
                return false;
            }
            break;
        }
    }

    //
    // If we have a sync master we now know the timestamp we want from the other cameras.
    // If we don't have a sync master we simply sync everything to the first camera.
    //
    for (auto cam : cameras) { //SUBORDINATE or STANDALONE
        if (!cam->is_sync_master()) {
            uint64_t this_cam_timestamp = cam->wait_for_captured_frameset(timestamp);
            if (this_cam_timestamp == 0) {
                _log_warning("failed to capture frameset for camera " + cam->serial);
                return false;
            }
            if (timestamp == 0) {
                timestamp = this_cam_timestamp;
            }
        }
    }
    return true;
}

bool K4APlaybackCapture::seek(uint64_t timestamp) {
    for (auto cam : cameras) { //SUBORDINATE or STANDALONE
        if (cam->seek(timestamp) != true) {
            _log_error("Camera " + cam->serial + " failed to seek to timestamp " + std::to_string(timestamp));
            return false;
        }
    }

    return true;
}
