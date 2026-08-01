//
//  multiFrame.cpp
//
//  Created by Fons Kuijk on 23-04-18
//
#include <cstdlib>

// Define to get debug prints. Mustr also be enabled through
// debug flag in cameraconfig.
#undef CWIPC_DEBUG
#undef CWIPC_DEBUG_THREAD
#define CWIPC_DEBUG_SYNC

// Only for RGB and Depth metadata: we have the option of mapping depth to color or color to depth.
#define MAP_DEPTH_IMAGE_TO_COLOR_IMAGE

// This is the dll source, so define external symbols as dllexport on windows.

#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_REALSENSE2_EXPORT __declspec(dllexport)
#endif

#include <librealsense2/rsutil.h>

#include "RS2Config.hpp"
#include "RS2BaseCamera.hpp"

RS2BaseCamera::RS2BaseCamera(rs2::context& _ctx, RS2CaptureConfig& configuration, RS2CaptureMetadataConfig& _metadata, int _camera_index)
: CwipcBaseCamera("cwipc_realsense::RS2BaseCamera: " + configuration.all_camera_configs[_camera_index].serial, "realsense2"),
  pointSize(0), 
  camera_index(_camera_index),
  serial(configuration.all_camera_configs[_camera_index].serial),
  camera_sync_is_master(configuration.sync.sync_master_serial == serial),
  camera_stopped(true),
  camera_config(configuration.all_camera_configs[_camera_index]),
  filtering(configuration.filtering),
  processing(configuration.processing),
  hardware(configuration.hardware),
  metadata(_metadata),
  current_pcl_pointcloud(nullptr),
  processing_frame_queue(1),
  camera_pipeline(_ctx),
  camera_started(false),
  camera_processing_thread(nullptr),
  rs2filter_align_color_to_depth(RS2_STREAM_DEPTH),
  rs2filter_align_depth_to_color(RS2_STREAM_COLOR),
  debug(configuration.debug),
  prefer_color_timing(configuration.prefer_color_timing)
{
    if (debug) _log_debug("Creating camera");
    if (configuration.record_to_directory != "") {
        record_to_file = configuration.record_to_directory + "/" + serial + ".bag";
    }

}

RS2BaseCamera::~RS2BaseCamera() {
    if (debug) _log_debug("Destroying camera");
    assert(camera_stopped);
}

bool RS2BaseCamera::pre_start_all_cameras() {
    if (!_init_filters()) {
        return false;
    }
    if (!_init_hardware_for_this_camera()) {
        return false;
    }
#if 0
    if (!_init_skeleton_tracker()) {
        return false;
    }
#endif
    return true;
}

// Configure and initialize caputuring of one camera
bool RS2BaseCamera::start_camera() {
    assert(camera_stopped);
    assert(!camera_started);
    assert(camera_processing_thread == nullptr);
    rs2::config cfg;
    if (debug) _log_debug("Starting pipeline");
    _init_pipeline_for_this_camera(cfg);
    rs2::pipeline_profile profile = camera_pipeline.start(cfg);   // Start streaming with the configuration just set
    _post_start_this_camera(profile);
    _computePointSize(profile);
    camera_started = true;
    end_of_stream_reached = false;
    return true;
}

void RS2BaseCamera::start_camera_streaming() {
    assert(camera_stopped);
    camera_stopped = false;
    _start_capture_thread();
    _start_processing_thread();
}

void RS2BaseCamera::pre_stop_camera() {
    if (uses_recorder) {
        rs2::pipeline_profile profile = camera_pipeline.get_active_profile();
        rs2::device dev = profile.get_device();
        rs2::recorder recorder = dev.as<rs2::recorder>();
        if (!recorder) {
            _log_error("pre_stop_camera: uses_recorder is true but no rs2::recorder");
        } else {
            recorder.pause();
        }
    }
    if (debug) _log_debug("pre-stop camera");
    camera_stopped = true;
}

void RS2BaseCamera::stop_camera() {
    // Tell the grabber and processor thread that we want to stop.
    if (debug) _log_debug("stop camera");
    camera_stopped = true;

    // Clear out the queues so we don't get into a deadlock
    
    while (true) {
        rs2::frameset tmp;
        bool ok = processing_frame_queue.try_wait_for_frame(&tmp, 1);
        if (!ok) break;
    }
    
    // Join and delete the processor thread
    if (camera_processing_thread) {
        camera_processing_thread->join();
    }

    delete camera_processing_thread;
    camera_processing_thread = nullptr;
    // stop the pipeline.
    if (camera_started) {
        if (debug) _log_debug("stop pipeline");
        camera_pipeline.stop();
    }

    camera_started = false;
    processing_done = true;
    processing_done_cv.notify_one();
    if (debug) _log_debug("camera stopped");
}

bool RS2BaseCamera::mapcolordepth(int x_c, int y_c, int *out2d)
{
    // First we need to get the four matrices.
    rs2::depth_frame depth_frame = current_processed_frameset.get_depth_frame();
    rs2::video_frame color_frame = current_processed_frameset.get_color_frame();
    rs2::video_stream_profile depth_profile = depth_frame.get_profile().as<rs2::video_stream_profile>();
    rs2::video_stream_profile color_profile = color_frame.get_profile().as<rs2::video_stream_profile>();
    rs2_intrinsics depth_intrinsics = depth_profile.get_intrinsics();
    rs2_intrinsics color_intrinsics = color_profile.get_intrinsics();
    rs2_extrinsics depth_to_color = depth_profile.get_extrinsics_to(color_profile);
    rs2_extrinsics color_to_depth = color_profile.get_extrinsics_to(depth_profile);
    const uint16_t* depth_data = (const uint16_t *)depth_frame.get_data();
    // Now we can convert the RGB x,y coordinates to depth x,y coordinates.
    float xy_color[2] { 
        float(x_c), 
        float(y_c)
        };
    float xy_depth[2];
    float min_distance = filtering.threshold_min_distance;
    float max_distance = filtering.threshold_max_distance;
    rs2_project_color_pixel_to_depth_pixel(
        xy_depth,
        depth_data,
        1000,
        min_distance,
        max_distance,
        &depth_intrinsics,
        &color_intrinsics,
        &color_to_depth,
        &depth_to_color,
        xy_color
    );
    out2d[0] = (int)xy_depth[0];
    out2d[1] = (int)xy_depth[1];
    return true;
}

bool RS2BaseCamera::map2d3d(int x_2d, int y_2d, int d_2d, float *out3d)
{
    float tmp3d[3] = {0, 0, 0};

    // We look up the depth value.
    // First we need to get the four matrices.
    rs2::depth_frame depth_frame = current_processed_frameset.get_depth_frame();
    rs2::video_frame color_frame = current_processed_frameset.get_color_frame();
    rs2::video_stream_profile depth_profile = depth_frame.get_profile().as<rs2::video_stream_profile>();
    rs2::video_stream_profile color_profile = color_frame.get_profile().as<rs2::video_stream_profile>();
    rs2_intrinsics depth_intrinsics = depth_profile.get_intrinsics();
    rs2_intrinsics color_intrinsics = color_profile.get_intrinsics();
    rs2_extrinsics depth_to_color = depth_profile.get_extrinsics_to(color_profile);
    rs2_extrinsics color_to_depth = color_profile.get_extrinsics_to(depth_profile);
    const uint16_t* depth_data = (const uint16_t *)depth_frame.get_data();
    // Now we can convert the RGB x,y coordinates to depth x,y coordinates.
    float xy_color[2] { 
        float(x_2d), 
        float(y_2d)
        };
    float xy_depth[2];
    float min_distance = filtering.threshold_min_distance;
    float max_distance = filtering.threshold_max_distance;
    rs2_project_color_pixel_to_depth_pixel(
        xy_depth,
        depth_data,
        1000,
        min_distance,
        max_distance,
        &depth_intrinsics,
        &color_intrinsics,
        &color_to_depth,
        &depth_to_color,
        xy_color
    );
    // Now we can use uv_depth to find the correct depth value.
    uint16_t depth_i = depth_data[(int)xy_depth[1]*depth_intrinsics.width + (int)xy_depth[0]];
    rs2_deproject_pixel_to_point(tmp3d, &depth_intrinsics, xy_depth, depth_i / 1000.0);
    cwipc_pcl_point pt = { tmp3d[0], tmp3d[1], tmp3d[2], 0, 0, 0, 0 };
    _transform_point_cam_to_world(pt);
    out3d[0] = pt.x;
    out3d[1] = pt.y;
    out3d[2] = pt.z;
    return true;
}

void RS2BaseCamera::get_camera_hardware_parameters(RS2CameraHardwareConfig& output) {
    rs2::pipeline_profile profile = camera_pipeline.get_active_profile();
    rs2::device dev = profile.get_device();
        rs2::depth_sensor depth_sensor = dev.first<rs2::depth_sensor>();
        rs2::color_sensor color_sensor = dev.first<rs2::color_sensor>();

        // Some parameters are easiest to get from our config, as they won't
        // change anyway
        output.color_width = hardware.color_width;
        output.color_height = hardware.color_height;
        output.depth_width = hardware.depth_width;
        output.depth_height = hardware.depth_height;
        output.fps = hardware.fps;

        bool auto_color_exposure = (bool)color_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        int color_exposure = (int)color_sensor.get_option(RS2_OPTION_EXPOSURE);
        output.color_exposure = auto_color_exposure ? -color_exposure : color_exposure;  

        int color_gain = (int)color_sensor.get_option(RS2_OPTION_GAIN);
        output.color_gain = color_gain;
        
        bool auto_whitebalance = (bool)color_sensor.get_option(RS2_OPTION_ENABLE_AUTO_WHITE_BALANCE);
        int whitebalance = (int)color_sensor.get_option(RS2_OPTION_WHITE_BALANCE);
        output.whitebalance = auto_whitebalance ? -whitebalance : whitebalance;  

        output.backlight_compensation = (int)color_sensor.get_option(RS2_OPTION_BACKLIGHT_COMPENSATION);

        bool auto_depth_exposure = (bool)depth_sensor.get_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE);
        int depth_exposure = (int)depth_sensor.get_option(RS2_OPTION_EXPOSURE);
        output.depth_exposure = auto_depth_exposure ? -depth_exposure : depth_exposure;

        int depth_gain = (int)depth_sensor.get_option(RS2_OPTION_GAIN);
        output.depth_gain = depth_gain;

        output.laser_power = (int)depth_sensor.get_option(RS2_OPTION_LASER_POWER);

#ifdef xxxjack_disabled
        // It seems the visual preset is always returned as 0 (also seen in realsense-viewer)
        // so we don't try to get the value.
        output.visual_preset = (int)depth_sensor.get_option(RS2_OPTION_VISUAL_PRESET);
#endif
}

bool RS2BaseCamera::match_camera_hardware_parameters(RS2CameraHardwareConfig& wanted) {
    return 
        wanted.fps == hardware.fps &&

        wanted.color_height == hardware.color_height &&
        wanted.color_width == hardware.color_width &&
        wanted.color_exposure == hardware.color_exposure &&
        wanted.color_gain == hardware.color_gain &&
        wanted.backlight_compensation == hardware.backlight_compensation &&
        wanted.whitebalance == hardware.whitebalance &&

        wanted.depth_exposure == hardware.depth_exposure &&
        wanted.depth_gain == hardware.depth_gain &&
        wanted.depth_height == hardware.depth_height &&
        wanted.depth_width == hardware.depth_width &&
        wanted.visual_preset == hardware.visual_preset &&
        wanted.laser_power == hardware.laser_power;
}

uint64_t RS2BaseCamera::wait_for_captured_frameset(uint64_t minimum_timestamp) {
    if (camera_stopped) return 0;
    uint64_t resultant_timestamp = 0;
    if (minimum_timestamp > 0 && previous_captured_frameset) {
        uint64_t previous_timestamp = previous_captured_frameset.get_depth_frame().get_timestamp();
        if (previous_timestamp > minimum_timestamp) {
            current_captured_frameset = previous_captured_frameset;
#ifdef CWIPC_DEBUG_SYNC
            if (debug) {
                uint64_t depth_timestamp = current_captured_frameset.get_depth_frame().get_timestamp();
                uint64_t color_timestamp = current_captured_frameset.get_color_frame().get_timestamp();
                if (debug) _log_debug("wait_for_captured_frameset: dts=" + std::to_string(depth_timestamp) + ", cts=" + std::to_string(color_timestamp) + " (previous)");
            }
#endif
            return previous_timestamp;
        }
    }
    do {
        if (current_captured_frameset) {
            previous_captured_frameset = current_captured_frameset;
        } else {
            // First frame. So fill previous_captured_frameset.
            previous_captured_frameset = _wait_for_frames_from_pipeline();
        }
        current_captured_frameset = _wait_for_frames_from_pipeline();
        if (end_of_stream_reached) {
            return resultant_timestamp;
        }
        rs2::depth_frame depth_frame = current_captured_frameset.get_depth_frame();
        if (depth_frame.get() == nullptr) {
            _log_warning("wait_for_captured_frameset: no depth frame in frameset");
            return resultant_timestamp;
        }
        // We now have both a previous and a current captured frameset.
        if(prefer_color_timing) {
            // If they both have the same depth timestamp we capture another one.
            if (depth_frame.get_timestamp() == previous_captured_frameset.get_depth_frame().get_timestamp()) {
                previous_captured_frameset = current_captured_frameset;
                current_captured_frameset = _wait_for_frames_from_pipeline();
                if (end_of_stream_reached) {
                    return resultant_timestamp;
                }
                depth_frame = current_captured_frameset.get_depth_frame();
                if (depth_frame.get() == nullptr) {
                    _log_warning("wait_for_captured_frameset: no depth frame in frameset");
                    return resultant_timestamp;
                }
            }
        }
        resultant_timestamp = (uint64_t)depth_frame.get_timestamp();
    } while(resultant_timestamp < minimum_timestamp);
#ifdef CWIPC_DEBUG_SYNC
    if (debug) {
        uint64_t depth_timestamp = current_captured_frameset.get_depth_frame().get_timestamp();
        uint64_t color_timestamp = current_captured_frameset.get_color_frame().get_timestamp();
        if (debug) _log_debug("wait_for_captured_frameset: dts=" + std::to_string(depth_timestamp) + ", cts=" + std::to_string(color_timestamp));
    }
#endif
    return resultant_timestamp;
}

void RS2BaseCamera::process_pointcloud_from_frameset() {
#ifdef CWIPC_DEBUG_SYNC
    if (debug) {
        uint64_t depth_timestamp = current_captured_frameset.get_depth_frame().get_timestamp();
        uint64_t color_timestamp = current_captured_frameset.get_color_frame().get_timestamp();
        if (debug) _log_debug("process_pointcloud_from_frameset: dts=" + std::to_string(depth_timestamp) + ", cts=" + std::to_string(color_timestamp));
    }
#endif
    processing_frame_queue.enqueue(current_captured_frameset);
}

void RS2BaseCamera::wait_for_pointcloud_processed() {
    std::unique_lock<std::mutex> lock(processing_mutex);
    processing_done_cv.wait(lock, [this]{ return processing_done; });
    processing_done = false;
}

void RS2BaseCamera::save_frameset_metadata(cwipc_pointcloud *pc)
{
    if (!metadata.want_depth && !metadata.want_rgb && !metadata.want_timestamps && !metadata.want_camera_specs) return;
    std::unique_lock<std::mutex> lock(processing_mutex);

    auto aligned_frameset = current_processed_frameset;
    if (aligned_frameset.size() == 0) return;
    rs2::video_frame color_image = aligned_frameset.get_color_frame();
    rs2::video_frame depth_image = aligned_frameset.get_depth_frame();
        
    if (metadata.want_timestamps) {
        
        int64_t depth_framenum = depth_image.get_frame_number();
        int64_t depth_timestamp = depth_image.get_timestamp();
        int32_t depth_clock = depth_image.get_frame_timestamp_domain();
        int64_t color_framenum = color_image.get_frame_number();
        int64_t color_timestamp = color_image.get_timestamp();
        int32_t color_clock = color_image.get_frame_timestamp_domain();
        std::string timestamp_data = 
            "depth_framenum=" + std::to_string(depth_framenum) +
            ",depth_timestamp=" + std::to_string(depth_timestamp) +
            ",depth_clock=" + std::to_string(depth_clock) +
            ",color_framenum=" + std::to_string(color_framenum) +
            ",color_timestamp=" + std::to_string(color_timestamp) +
            ",color_clock=" + std::to_string(color_clock);
        // xxxjack the following code is wrong. It gets the _current_ file position, not the file position
        // we were at when we read this frame. 
        rs2::playback playback = camera_pipeline.get_active_profile().get_device().as<rs2::playback>();
        if (playback) {
            timestamp_data += ",filetime_ns=" + std::to_string(playback.get_position());
        }
        cwipc_metadata *ap = pc->access_metadata();
        std::string name = "timestamps." + serial;
        ap->_add(name, timestamp_data, nullptr, 0, ::free);
    }
    if (metadata.want_rgb) {
        std::string name = "rgb." + serial;
        //const void* pointer = color.get_data();
        const size_t size = color_image.get_data_size();
        int width = color_image.get_width();
        int height = color_image.get_height();
        int stride = color_image.get_stride_in_bytes();
        int bpp = color_image.get_bytes_per_pixel();

        std::string description =
            "width="+std::to_string(width)+
            ",height="+std::to_string(height)+
            ",stride="+std::to_string(stride)+
            ",bpp="+std::to_string(bpp)+
            ",format="+std::string(color_format);

        void* pointer = malloc(size);

        if (pointer) {
            memcpy(pointer, color_image.get_data(), size);
            cwipc_metadata *ap = pc->access_metadata();
            ap->_add(name, description, pointer, size, ::free);
        }
    }

    if (metadata.want_depth) {
        std::string name = "depth." + serial;
        const size_t size = depth_image.get_data_size();
        int width = depth_image.get_width();
        int height = depth_image.get_height();
        int stride = depth_image.get_stride_in_bytes();
        int bpp = depth_image.get_bytes_per_pixel();

        std::string description =
            "width="+std::to_string(width)+
            ",height="+std::to_string(height)+
            ",stride="+std::to_string(stride)+
            ",bpp="+std::to_string(bpp)+
            ",format="+std::string(depth_format);

        void* pointer = malloc(size);

        if (pointer) {
            memcpy(pointer, depth_image.get_data(), size);
            cwipc_metadata *ap = pc->access_metadata();
            ap->_add(name, description, pointer, size, ::free);
        }
    }

    if (metadata.want_camera_specs) {
        rs2::pipeline_profile profile = camera_pipeline.get_active_profile();
        auto color_stream = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
        rs2_intrinsics intrinsics = color_stream.get_intrinsics();

        // Extract the required specs
        RS2CameraMetadataCameraSpecs* specs = (RS2CameraMetadataCameraSpecs*)malloc(sizeof(RS2CameraMetadataCameraSpecs));
        specs->focal_length_x = intrinsics.fx;
        specs->focal_length_y = intrinsics.fy;
        specs->principal_point_x = intrinsics.ppx;
        specs->principal_point_y = intrinsics.ppy;
        specs->color_image_width = intrinsics.width;
        specs->color_image_height = intrinsics.height;
        specs->near_plane = 0.1f;
        specs->far_plane = 10.0f;

        // Save the specs in the meta-data
        const std::string name = "camera." + serial;
        cwipc_metadata* ap = pc->access_metadata();
        ap->_add(name, "", (void*)specs, sizeof(RS2CameraMetadataCameraSpecs), ::free);
    }
}

bool RS2BaseCamera::_init_filters() {
    if (filtering.do_decimation) {
        rs2filter_decimation.set_option(RS2_OPTION_FILTER_MAGNITUDE, filtering.decimation_magnitude);
    }

    if (filtering.do_threshold) {
        rs2filter_threshold.set_option(RS2_OPTION_MIN_DISTANCE, filtering.threshold_min_distance);
        rs2filter_threshold.set_option(RS2_OPTION_MAX_DISTANCE, filtering.threshold_max_distance);
    }

    if (filtering.do_spatial) {
        rs2filter_spatial.set_option(RS2_OPTION_FILTER_MAGNITUDE, filtering.spatial_magnitude);
        rs2filter_spatial.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, filtering.spatial_smooth_alpha);
        rs2filter_spatial.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, filtering.spatial_smooth_delta);
        rs2filter_spatial.set_option(RS2_OPTION_HOLES_FILL, filtering.spatial_holes_fill);
    }

    if (filtering.do_temporal) {
        rs2filter_temporal.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, filtering.temporal_smooth_alpha);
        rs2filter_temporal.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, filtering.temporal_smooth_delta);
        // Note by Jack: the following is correct. The temporal persistency is set with the HOLES_FILL parameter...
        rs2filter_temporal.set_option(RS2_OPTION_HOLES_FILL, filtering.temporal_persistency);
    }
    if (filtering.do_hole_filling) {
        rs2filter_hole_filling.set_option(RS2_OPTION_HOLES_FILL, filtering.hole_filling_mode);
    }
    return true;
}

void RS2BaseCamera::_apply_filters_to_frameset(rs2::frameset& processing_frameset) {
    // First we apply the filters implemented by librealsense.
    _apply_rs2_filters(processing_frameset);
    // Apply mapping between color and depth.
    if (filtering.map_color_to_depth == 1) {
        processing_frameset = rs2filter_align_color_to_depth.process(processing_frameset);
    } else if (filtering.map_color_to_depth == 0) {
        processing_frameset = rs2filter_align_depth_to_color.process(processing_frameset);
    } else {
        // map_color_to_depth == -1 means no mapping t all.
    }
}

void RS2BaseCamera::_apply_rs2_filters(rs2::frameset& processing_frameset) {
    // Let's first see which filtering we want to do that is implemented in librealsense.
    // Some filters operate on the disparity, so we have to convert between depth and disparity:
    bool do_rs2_disparity = filtering.do_spatial || filtering.do_temporal || filtering.do_hole_filling;
    // Some more filters operate on depth:
    bool do_rs2_depth = do_rs2_disparity || filtering.do_decimation || filtering.do_threshold;;
    if (do_rs2_depth) {
    
        if (filtering.do_decimation) {
            processing_frameset = rs2filter_decimation.process(processing_frameset);
        }

        if (filtering.do_threshold) {
            processing_frameset = rs2filter_threshold.process(processing_frameset);
        }

        if (do_rs2_disparity) {
            processing_frameset = rs2filter_depth_to_disparity.process(processing_frameset);

            if (filtering.do_spatial) {
                processing_frameset = rs2filter_spatial.process(processing_frameset);
            }

            if (filtering.do_temporal) {
                processing_frameset = rs2filter_temporal.process(processing_frameset);
            }
            if (filtering.do_hole_filling) {
                processing_frameset = rs2filter_hole_filling.process(processing_frameset);
            }

            processing_frameset = rs2filter_disparity_to_depth.process(processing_frameset);
        }
    }
}

void RS2BaseCamera::_start_processing_thread() {
    camera_processing_thread = new std::thread(&RS2BaseCamera::_processing_thread_main, this);
    _cwipc_setThreadName(camera_processing_thread, L"cwipc_realsense2::RS2BaseCamera::camera_processing_thread");
}

void RS2BaseCamera::_processing_thread_main() {
    if (debug) _log_debug_thread("frame processing thread started");

    while(!camera_stopped) {
            //
            // Get the frameset we need to turn into a point cloud
            ///
        rs2::frameset processing_frameset;
        bool ok = processing_frame_queue.try_wait_for_frame(&processing_frameset, frame_timeout_ms);
        if (end_of_stream_reached) break;
        if (!ok) {
            if (waiting_for_capture) _log_warning("processing thread dequeue timeout");
            continue;
        }
        waiting_for_capture = false;
#ifdef CWIPC_DEBUG_SYNC
        if (debug) {
            uint64_t depth_timestamp = processing_frameset.get_depth_frame().get_timestamp();
            uint64_t color_timestamp = processing_frameset.get_color_frame().get_timestamp();
            if (debug) _log_debug("camera_processing_thread: dts=" + std::to_string(depth_timestamp) + ", cts=" + std::to_string(color_timestamp));
        }
#endif
        if (debug) _log_debug_thread("processing thread got frameset");

        std::lock_guard<std::mutex> lock(processing_mutex);
        //
        // For realsense, we apply most filters to the frameset.
        // Do that first. Keep the current_processed_frameset, which
        // may be used later for mapping coordinates.
        //
        _apply_filters_to_frameset(processing_frameset);

        current_processed_frameset = processing_frameset;

        rs2::depth_frame depth = processing_frameset.get_depth_frame();
        rs2::video_frame color = processing_frameset.get_color_frame();
        
        assert(depth);
        assert(color);
        //
        // Do processing on depth image.
        //
        if (processing.depth_x_erosion > 0 || processing.depth_y_erosion > 0) {
            _erode_depth(depth, processing.depth_x_erosion, processing.depth_y_erosion);
        }
        if (debug) _log_debug(std::string("Processing frame:") +
                   " depth: " + std::to_string(depth.get_width()) + "x" + std::to_string(depth.get_height()) +
                   " color: " + std::to_string(color.get_width()) + "x" + std::to_string(color.get_height()));

        //
        // generate point cloud.
        //
        cwipc_pcl_pointcloud new_pointcloud = _generate_point_cloud(depth, color);

        if (new_pointcloud != nullptr) {
            if (new_pointcloud->size() == 0) {
                _log_trace("Captured empty pointcloud from camera");
            }
            current_pcl_pointcloud = new_pointcloud;
            //
            // Notify wait_for_pointcloud_processed that we're done.
            //
            processing_done = true;
            processing_done_cv.notify_one();
        }
        //
        // No cleanup of resources needed, librealsense takes care of that.
        //
    }
    // stop the pipeline.
    if (camera_started) {
        if (debug) _log_debug("frame processing thread stop pipeline");
        camera_pipeline.stop();
        camera_started = false;
    }

    if (debug) _log_debug_thread("frame processing thread stopped");
}

cwipc_pcl_pointcloud RS2BaseCamera::_generate_point_cloud(rs2::depth_frame depth, rs2::video_frame color) {
#ifndef cwipc_global_pointcloud_filter
    rs2::pointcloud rs2filter_depth_to_pointcloud;
#endif
    rs2filter_depth_to_pointcloud.map_to(color);
    rs2::points points = rs2filter_depth_to_pointcloud.calculate(depth);
    const rs2::vertex* vertices = points.get_vertices();
    const rs2::texture_coordinate* texture_coordinates = points.get_texture_coordinates();

    // Get some constants used later to map colors and such from rs2 to pcl pointclouds.
    const int texture_width = color.get_width();
    const int texture_height = color.get_height();
    const int texture_x_step = color.get_bytes_per_pixel();
    const int texture_y_step = color.get_stride_in_bytes();
    const unsigned char *texture_data = (unsigned char*)color.get_data();
    const uint8_t camera_label = (uint8_t)1 << camera_index;

    // Clear the previous pointcloud and pre-allocate space in the pointcloud (so we don't realloc)
    cwipc_pcl_pointcloud pcl_pointcloud = new_cwipc_pcl_pointcloud();
    pcl_pointcloud->clear();
    pcl_pointcloud->reserve(points.size());

    // Make PointCloud
    float height_min = processing.height_min;
    float height_max = processing.height_max;
    bool do_height_filtering = height_min < height_max;
    bool do_greenscreen_removal = processing.greenscreen_removal;
    bool do_radius_filtering = processing.radius_filter > 0;
    for (int i = 0; i < points.size(); i++) {
        // Skip points with z=0 (they don't exist)
        if (vertices[i].z == 0) {
            continue;
        }

        cwipc_pcl_point pt(vertices[i].x, vertices[i].y, vertices[i].z);
        _transform_point_cam_to_world(pt);

        if (do_height_filtering && (pt.y < height_min || pt.y > height_max)) {
            continue;
        }
        if (do_radius_filtering && !isPointInRadius(pt, processing.radius_filter)) {
            continue;
        }
        float u = texture_coordinates[i].u;
        float v = texture_coordinates[i].v;

        int texture_x = int(0.5 + u*texture_width);
        int texture_y = int(0.5 + v*texture_height);

        // Unsure whether this ever happens: out-of-bounds u/v points are skipped
        if (texture_x <= 0 || texture_x >= texture_width-1) {
            continue;
        }

        if (texture_y <= 0 || texture_y >= texture_height-1) {
            continue;
        }

        int idx = texture_x * texture_x_step + texture_y * texture_y_step;
        pt.r = texture_data[idx];
        pt.g = texture_data[idx + 1];
        pt.b = texture_data[idx + 2];

        // Unexpectedly, this does happen: 100% black points don't actually exist.
        if (pt.r == 0 && pt.g == 0 && pt.b == 0) {
            continue;
        }

        pt.a = camera_label;

        if (!do_greenscreen_removal || isNotGreen(&pt)) {
            // chromakey removal
            pcl_pointcloud->push_back(pt);
        }
    }
    return pcl_pointcloud;
}

void RS2BaseCamera::_post_start_this_camera(rs2::pipeline_profile& profile) {
    rs2::device dev = profile.get_device();

    // Obtain actual serial number and fps. Most important for playback cameras, but also useful for
    // live cameras (because the user program can obtain correct cameraconfig data with get_config()).
    // Keep actual serial number, also in cameraconfig
    serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
    camera_config.serial = serial;
    std::vector<rs2::sensor> sensors = dev.query_sensors();
    int depth_fps = hardware.fps;
    int depth_width = hardware.depth_width;
    int depth_height = hardware.depth_height;
    int color_fps = hardware.fps;
    int color_width = hardware.color_width;
    int color_height = hardware.color_height;
    for(rs2::sensor sensor : sensors) {
        std::vector<rs2::stream_profile> streams = sensor.get_active_streams();
        for(rs2::stream_profile stream : streams) {
            rs2::video_stream_profile vstream = stream.as<rs2::video_stream_profile>();

            if (vstream.stream_type() == RS2_STREAM_DEPTH) {
                depth_fps = vstream.fps();
                depth_width = vstream.width();
                depth_height = vstream.height();
                depth_format = rs2_format_to_string(vstream.format());

            }
            if (stream.stream_type() == RS2_STREAM_COLOR) {
                color_fps = vstream.fps();
                color_width = vstream.width();
                color_height = vstream.height();
                color_format = rs2_format_to_string(vstream.format());
            }
        }
    }
    if (depth_fps != color_fps) {
        _log_warning("Depth and color fps differ: depth " + std::to_string(depth_fps) + " color " + std::to_string(color_fps) + ". Using depth fps.");
    }
    hardware.fps = depth_fps;
    hardware.depth_width = depth_width;
    hardware.depth_height = depth_height;
    hardware.color_width = color_width;
    hardware.color_height = color_height;
}

rs2::frameset RS2BaseCamera::_wait_for_frames_from_pipeline() {
    waiting_for_capture = true;
    rs2::frameset frames;
    try {
        frames = camera_pipeline.wait_for_frames(frame_timeout_ms);
#ifdef CWIPC_DEBUG_SYNC
        if (debug) {
            uint64_t depth_timestamp = frames.get_depth_frame().get_timestamp();
            uint64_t color_timestamp = frames.get_color_frame().get_timestamp();
            if (debug) _log_debug("wait_for_frames: dts=" + std::to_string(depth_timestamp) + ", cts=" + std::to_string(color_timestamp));
        }
#endif
    } catch(const rs2::error &e) {
        std::string what(e.what());
        if (what.find("Frame didn't arrive within") == std::string::npos) {
            _log_error("wait_for_frames: rs2::error " + what);
        }
        end_of_stream_reached = true;
    }
    return frames;
}

void RS2BaseCamera::_erode_depth(rs2::depth_frame depth_frame, int x_delta, int y_delta) {
    uint16_t *depth_values = (uint16_t *)depth_frame.get_data();
    assert(depth_frame.get_bytes_per_pixel() == 2);
    int width = depth_frame.get_width();
    int height = depth_frame.get_height();
    int16_t min_depth = (int16_t)(filtering.threshold_min_distance * 1000);
    int16_t max_depth = (int16_t)(filtering.threshold_max_distance * 1000);
    int16_t *z_values = (int16_t *)malloc(width * height*sizeof(int16_t));

    // Pass one: Copy Z values to temporary buffer.
    memcpy(z_values, depth_values, width * height*sizeof(int16_t));

    // Pass two: loop for zero pixels in temp buffer, and clear out x/y pixels adjacent in depth buffer
    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            if (z_values[x + y*width] != 0) {
               continue;
            }

            // Zero depth at (x, y). Clear out pixels
            for (int ix = x - x_delta; ix <= x + x_delta; ix++) {
                if (ix < 0 || ix >= width) {
                    continue;
                }

                int i_pc = (ix + y * width);
                depth_values[i_pc] = 0;
            }
            for (int iy = y - y_delta; iy <= y + y_delta; iy++) {
                if (iy < 0 || iy >= height) {
                    continue;
                }

                int i_pc = (x + iy * width);
                depth_values[i_pc] = 0;
            }
        }
    }

    free(z_values);
}

void RS2BaseCamera::_computePointSize(rs2::pipeline_profile profile) {
    // Get the 3D distance between camera and (0,0,0) or use 1m if unreasonable
    float tx = (*camera_config.trafo)(0,3);
    float ty = (*camera_config.trafo)(1,3);
    float tz = (*camera_config.trafo)(2,3);
    float dist = sqrt(tx*tx + ty*ty + tz*tz);

    if (dist == 0) {
        dist = 1;
    }

    // Now get the intrinsics for the depth stream
    auto stream = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
    auto intrinsics = stream.get_intrinsics(); // Calibration data

    // Compute 2D coordinates of adjacent pixels in the middle of the field of view
    float pixel0[2], pixel1[2];
    pixel0[0] = hardware.depth_width / 2;
    pixel0[1] = hardware.depth_height / 2;
    if (filtering.do_decimation) {
        pixel1[0] = pixel0[0] + filtering.decimation_magnitude;
        pixel1[1] = pixel0[1] + filtering.decimation_magnitude;
    } else {
        pixel1[0] = pixel0[0] + 1;
        pixel1[1] = pixel0[1] + 1;
    }

    // Deproject to get 3D distance
    float point0[3], point1[3];
    rs2_deproject_pixel_to_point(point0, &intrinsics, pixel0, dist);
    rs2_deproject_pixel_to_point(point1, &intrinsics, pixel1, dist);
    float rv = sqrt(pow(point1[0]-point0[0], 2)+pow(point1[1]-point0[1], 2)+pow(point1[2]-point0[2], 2));
    pointSize = rv;
}

void RS2BaseCamera::_transform_point_cam_to_world(cwipc_pcl_point& pt) {
    float x = (*camera_config.trafo)(0,0)*pt.x + (*camera_config.trafo)(0,1)*pt.y + (*camera_config.trafo)(0,2)*pt.z + (*camera_config.trafo)(0,3);
    float y = (*camera_config.trafo)(1,0)*pt.x + (*camera_config.trafo)(1,1)*pt.y + (*camera_config.trafo)(1,2)*pt.z + (*camera_config.trafo)(1,3);
    float z = (*camera_config.trafo)(2,0)*pt.x + (*camera_config.trafo)(2,1)*pt.y + (*camera_config.trafo)(2,2)*pt.z + (*camera_config.trafo)(2,3);
    pt.x = x;
    pt.y = y;
    pt.z = z;
}

