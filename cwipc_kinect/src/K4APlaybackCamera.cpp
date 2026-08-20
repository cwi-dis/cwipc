//
//  multiFrame.cpp
//
//  Created by Fons Kuijk on 23-04-18
//
#include <cstdlib>

#include "K4APlaybackCamera.hpp"

#undef CWIPC_DEBUG

K4APlaybackCamera::K4APlaybackCamera(Type_api_camera _handle, K4ACaptureConfig& configuration, K4ACaptureMetadataConfig& metadata, int _camera_index)
:   K4ABaseCamera("cwipc_kinect: K4APlaybackCamera", _handle, configuration, metadata, _camera_index),
    capture_id(-1),
    current_frameset_timestamp(0),
    max_delay(0)
{
    if (debug) _log_debug("creating playback camera from file " + camera_config.filename);
    _init_filters();
}

uint64_t K4APlaybackCamera::wait_for_captured_frameset(uint64_t earliest_timestamp) {
    if (camera_stopped) {
        return 0;
    }
    bool ok = true;
    if (earliest_timestamp > 0) {
        ok = _capture_frame_no_earlier_than_timestamp(earliest_timestamp);
    } else {
        ok = _capture_next_valid_frame();
    }

    if (!ok) {
        // Any error message will have been printed by the methods above.
        return 0;
    }

    return current_frameset_timestamp;
}

bool K4APlaybackCamera::seek(uint64_t timestamp) {
    uint64_t first_t = file_config.start_timestamp_offset_usec;
    uint64_t duration_t = k4a_playback_get_recording_length_usec(camera_handle);

    if (timestamp > first_t + duration_t) {
        _log_error("seek timestamp " + std::to_string(timestamp) + " out of range start=" + std::to_string(first_t) + ", stop=" + std::to_string(first_t + duration_t));
        return false;
    }

    if (k4a_playback_seek_timestamp(camera_handle, timestamp, K4A_PLAYBACK_SEEK_DEVICE_TIME) != K4A_RESULT_SUCCEEDED) {
        return false;
    } else {
        return true;
    }
}

bool K4APlaybackCamera::_capture_next_valid_frame() {
    k4a_stream_result_t stream_result;
    // Read the next capture into memory
    bool succeeded = false;

    while (!succeeded) {
        if (camera_stopped) {
            return false;
        }

        assert(camera_handle);

        if (current_captured_frameset != NULL) {
            k4a_capture_release(current_captured_frameset);
            current_captured_frameset = nullptr;
        }
        waiting_for_capture = true;
        stream_result = k4a_playback_get_next_capture(camera_handle, &current_captured_frameset);
        if (stream_result == K4A_STREAM_RESULT_EOF) {
            end_of_stream_reached = true; // xxxjack note that this means eof is true *after all frames have been processed*.
            if (current_frameset_timestamp == 0) {
                _log_warning("Recording file is empty.");
            } else {
                _log_trace("Recording file reached EOF at frame " + std::to_string(capture_id));
            }
            return false;
        }

        if (stream_result == K4A_STREAM_RESULT_FAILED) {
            _log_error("First capture failed");
            return false;
        }

        capture_id++;
        k4a_image_t color = k4a_capture_get_color_image(current_captured_frameset);
        k4a_image_t depth = k4a_capture_get_depth_image(current_captured_frameset);

        succeeded = (color != nullptr && depth != nullptr);
        if (color == NULL) {
            _log_warning("Color is missing in frame " + std::to_string(capture_id));
        } else if (depth == NULL) {
            _log_warning("Depth is missing in frame " + std::to_string(capture_id));
        }
        // xxxjack stop-gap: suddenly sometimes color is missing from the first frame.
        // Try to capture again.
        if (!succeeded) {
            continue;
        }
        // xxxjack note that this code uses *color* frame timestamp...
        current_frameset_timestamp = k4a_image_get_device_timestamp_usec(color);
        if (debug) _log_debug("Captured frame " + std::to_string(capture_id) + " with timestamp " + std::to_string(current_frameset_timestamp) + " from file " + camera_config.filename);
        k4a_image_release(color);
        k4a_image_release(depth);
    }

    return succeeded;
}

bool K4APlaybackCamera::_capture_frame_no_earlier_than_timestamp(uint64_t master_timestamp) {
    //check if current frame already satisfies the condition
    if (current_captured_frameset != NULL && (current_frameset_timestamp > master_timestamp)) {
        // Even if the current frame is too far in the future we use it.
        _log_warning("Reusing frame with timestamp " + std::to_string(current_frameset_timestamp) + " for master timestamp " +  std::to_string(master_timestamp));
        return true;
    }

    //otherwise start process to find a frame that satisfies the condition.
    while (true) {
        if (!_capture_next_valid_frame()) {
            return false;
        }

        if (current_frameset_timestamp > master_timestamp) {
            if (current_frameset_timestamp > (master_timestamp + max_delay)) {
                // it is a future frame, we need to update master frame
                _log_warning("Using frame with timestamp " + std::to_string(current_frameset_timestamp) + ", too early by " + std::to_string(current_frameset_timestamp - master_timestamp));
                return true;
            } else {  
                return true;
            }
        }
    }
}

bool K4APlaybackCamera::start_camera() {
    // We don't have to start anything (opening the file did that) but
    // we do have to get the RGB<->D transformation.
    if (K4A_RESULT_SUCCEEDED != k4a_playback_get_calibration(camera_handle, &sensor_calibration)) {
        _log_error("Failed to k4a_device_get_calibration");
        camera_started = false;

        return false;
    }

    depth_to_color_extrinsics = sensor_calibration.extrinsics[0][1];
    transformation_handle = k4a_transformation_create(&sensor_calibration);

    if (depth_uv_mapping == NULL) { // generate depth_uv_mapping for the fast pc_gen v2
        _create_depth_uv_mapping(&sensor_calibration);
    }

    //Get file config for further use of the parameters.
    k4a_result_t result = k4a_playback_get_record_configuration(camera_handle, &file_config);
    if (result != K4A_RESULT_SUCCEEDED) {
        _log_error("Failed to get record configuration");
        return false;
    }

    camera_started = true;
    max_delay = 10 * 160; //we set a 160us delay between cameras to avoid laser interference. It is enough for 10x cameras
    return true;
}

void K4APlaybackCamera::stop_camera() {
    if (camera_stopped) {
        return;
    }

    camera_stopped = true;
    processing_frame_queue.try_enqueue(NULL);

    // Stop threads
    if (camera_processing_thread) {
        camera_processing_thread->join();
    }

    delete camera_processing_thread;
    camera_processing_thread = nullptr;

    if (camera_started) {
        // Nothing to stop for reading from file
        camera_started = false;
    }

    // Delete objects
    if (current_captured_frameset != NULL) {
        k4a_capture_release(current_captured_frameset);
        current_captured_frameset = NULL;
    }

    if (camera_handle) {
        k4a_playback_close(camera_handle);
        camera_handle = nullptr;
    }

    if (transformation_handle) {
        k4a_transformation_destroy(transformation_handle);
        transformation_handle = NULL;
    }

    processing_done = true;
    processing_done_cv.notify_one();

#ifdef CWIPC_WITH_KINECT_SKELETONS
    if (tracker_handle) {
        k4abt_tracker_destroy(tracker_handle);
        tracker_handle = nullptr;
    }
#endif
}

