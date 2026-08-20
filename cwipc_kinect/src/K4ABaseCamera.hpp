#pragma once

#include <string>
#include <thread>
#include <mutex>
#include <condition_variable>

#include <k4a/k4a.h>
#include <k4arecord/playback.h>
#include <k4arecord/record.h>
#ifdef CWIPC_WITH_KINECT_SKELETONS
#include <k4abt.h>
#endif
#include "cwipc_util/api_pcl.h"
#include "cwipc_kinect/api.h"
#include "cwipc_util/internal/capturers.hpp"

#include "K4AConfig.hpp"
#include "readerwriterqueue.h"

//opencv xxxnacho
#include <opencv2/opencv.hpp>

// Check which K4ABT version we have
# if K4ABT_VERSION_MAJOR >= 1 && K4ABT_VERSION_MINOR >= 1
#define K4ABT_SUPPORTS_MODEL_PATH
#endif

// Define to get (a little) debug prints
#undef CWIPC_DEBUG
#undef CWIPC_DEBUG_THREAD

typedef union {
    /** XY or array representation of vector */
    struct _bgra {
        uint8_t b; /**< b component of a vector */
        uint8_t g; /**< g component of a vector */
        uint8_t r; /**< r component of a vector */
        uint8_t a; /**< a component of a vector */
    } bgra;        /**< B, G, R, A representation of a vector */

    uint8_t v[4];  /**< Array representation of a vector */
} cwi_bgra_t;

/// Optionally uncompress color image.
/// For code simplicity this always returns a value k4a_image_t, even though it may be wrong.
k4a_image_t cwipc_k4a_uncompress_color_image(CwipcBaseCamera *cam, k4a_capture_t capture, k4a_image_t color_image);

template<typename Type_api_camera> 
class K4ABaseCamera : public CwipcBaseCamera {
public:
    K4ABaseCamera(const std::string& _Classname, Type_api_camera _handle, K4ACaptureConfig& _configuration, K4ACaptureMetadataConfig& _metadata, int _camera_index)
    :   CwipcBaseCamera(_Classname + ": " + _configuration.all_camera_configs[_camera_index].serial, "kinect"),
        camera_index(_camera_index),
        configuration(_configuration),
        serial(_configuration.all_camera_configs[_camera_index].serial),
        camera_sync_ismaster(serial == configuration.sync.sync_master_serial),
        camera_stopped(true),
        camera_config(_configuration.all_camera_configs[_camera_index]),
        filtering(_configuration.filtering),
        processing(_configuration.processing),
        hardware(_configuration.hardware),
        skeleton(_configuration.skeleton),
        metadata(_metadata),
        camera_handle(_handle),
        captured_frame_queue(1),
        processing_frame_queue(1),
        debug(_configuration.debug),
        camera_sync_inuse(configuration.sync.sync_master_serial != "")
    {

    }

    virtual ~K4ABaseCamera() {
        if (debug) _log_debug("Destroying camera");
        assert(camera_stopped);
#ifdef CWIPC_WITH_KINECT_SKELETONS
        if (tracker_handle) {
            k4abt_tracker_shutdown(tracker_handle);
            k4abt_tracker_destroy(tracker_handle);
            tracker_handle = nullptr;
        }
#endif
    }
    /// Step 1 in starting: tell the camera we are going to start. Called for all cameras.
    virtual bool pre_start_all_cameras() final {
        if (!_init_filters()) {
            return false;
        }
        if (!_init_hardware_for_this_camera()) {
            return false;
        }
        if (metadata.want_skeleton) {
            if (!_init_skeleton_tracker()) {
                return false;
            }
        }
        return true;
     }
    /// Step 2 in starting: starts the camera. Called for all cameras. 
    virtual bool start_camera() = 0;
    /// Step 3 in starting: starts the capturer. Called after all cameras have been started.
    virtual void start_camera_streaming() final {
        if (!camera_started) {
            return;
        }
        assert(camera_stopped);
        camera_stopped = false;
        _start_capture_thread();
        _start_processing_thread();

    }
    /// Step 4, called after all capturers have been started.
    virtual void post_start_all_cameras() final {}
    virtual void pre_stop_camera() final {}
    virtual void stop_camera() = 0;

    virtual bool is_sync_master() final {
        return camera_sync_ismaster;
    }

    virtual bool mapcolordepth(int x_c, int y_c, int* out2d) override final {
        // For Kinect the depth and color images have the same coordinate system.
        out2d[0] = x_c;
        out2d[1] = y_c;
        return true;
    }

    virtual bool map2d3d(int x_2d, int y_2d, int d_2d, float* out3d) override final
    {
        float depth = d_2d; // 1000.0; // Note: this comes out of the Depth image, so it is in millimeters

        k4a_float2_t* uv_factors_for_depth_table = (k4a_float2_t*)(void*)k4a_image_get_buffer(depth_uv_mapping);
        int width = k4a_image_get_width_pixels(depth_uv_mapping);
        int height = k4a_image_get_height_pixels(depth_uv_mapping);
        if (y_2d < 0 || y_2d >= height || x_2d < 0 || x_2d >= width) {
            _log_error("map2d3d: requested pixel out of bounds");
            return false;
        }
        int idx = y_2d * width + x_2d;
        cwipc_pcl_point point;
        point.x = uv_factors_for_depth_table[idx].xy.x * depth;
        point.y = uv_factors_for_depth_table[idx].xy.y * depth;
        point.z = depth;
        if (filtering.map_color_to_depth) {
            _transform_point_depth_to_color(point);
        }
        _transform_point_cam_to_world(point); //transforming from camera to world coordinates
        out3d[0] = point.x;
        out3d[1] = point.y;
        out3d[2] = point.z;
        return true;
    }

    /// Get current camera hardware parameters.
    /// xxxjack to be overridden for real cameras.
    virtual void get_camera_hardware_parameters(K4ACameraHardwareConfig& output) {
        output = hardware;
    }
    /// Return true if current hardware parameters of this camera match input.
    bool match_camera_hardware_parameters(K4ACameraHardwareConfig& input) {
        return (
            input.color_backlight_compensation == hardware.color_backlight_compensation &&
            input.color_brightness == hardware.color_brightness &&
            input.color_contrast == hardware.color_contrast &&
            input.color_exposure_time == hardware.color_exposure_time &&
            input.color_gain == hardware.color_gain &&
            input.color_height == hardware.color_height &&
            input.color_powerline_frequency == hardware.color_powerline_frequency &&
            input.color_saturation == hardware.color_saturation &&
            input.color_sharpness == hardware.color_sharpness &&
            input.color_whitebalance == hardware.color_whitebalance &&
            input.depth_height == hardware.depth_height &&
            input.fps == hardware.fps
        );
    }

public:
    /// Step 1 in capturing: wait for a valid frameset. Any image processing will have been done. 
    /// Returns timestamp of depth frame, or zero if none available.
    /// This ensures protected attribute current_captured_frameset is valid.
    virtual uint64_t wait_for_captured_frameset(uint64_t minimum_timestamp) = 0;
    /// Step 2: Forward the current_captured_frameset to the processing thread to turn it into a point cloud.
    virtual void process_pointcloud_from_frameset() final {
        assert(current_captured_frameset);

        if (!processing_frame_queue.try_enqueue(current_captured_frameset)) {
            _log_warning("processing frame queue full, dropping frame");
            k4a_capture_release(current_captured_frameset);
        }

        current_captured_frameset = NULL;
    }
    /// Step 3: Wait for the point cloud processing.
    /// After this, current_pcl_pointcloud will be valid.
    virtual void wait_for_pointcloud_processed() final {
        std::unique_lock<std::mutex> lock(processing_mutex);
        processing_done_cv.wait(lock, [this] { return processing_done; });
        processing_done = false;
    }
    /// Step 4: borrow a pointer to the point cloud just created, as a PCL point cloud.
    /// The capturer will use this to populate the resultant cwipc_pointcloud point cloud with points
    /// from all cameras.
    virtual cwipc_pcl_pointcloud access_current_pcl_pointcloud() final {
        return current_pcl_pointcloud;
    }
    /// Step 5: Save metadata from frameset into given cwipc_pointcloud object.
    void save_frameset_metadata(cwipc_pointcloud* pc) {
        if (current_captured_frameset == nullptr) {
            _log_error("save_frameset_metadata: current_captured_frameset is NULL");
            return;
        }
        // xxxjack do we need to lock current_frameset here?
        if (metadata.want_rgb || metadata.want_depth) {
            k4a_image_t color_image = k4a_capture_get_color_image(current_captured_frameset);
            k4a_image_t depth_image = k4a_capture_get_depth_image(current_captured_frameset);
            if (color_image == nullptr || depth_image == nullptr) {
                _log_error("Failed to get color or depth image from capture for metadata");
                return;
            }
            int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
            int color_image_height_pixels = k4a_image_get_height_pixels(color_image);
            int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
            int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);

            if (metadata.want_rgb) {
                std::string name = "rgb." + serial;
                color_image = cwipc_k4a_uncompress_color_image(this, current_captured_frameset, color_image);
                if (filtering.map_color_to_depth) {
                    k4a_image_t transformed_color_image = NULL;
                    k4a_result_t status;

                    status = k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
                        depth_image_width_pixels,
                        depth_image_height_pixels,
                        depth_image_width_pixels * 4 * (int)sizeof(uint8_t),
                        &transformed_color_image
                    );

                    if (status != K4A_RESULT_SUCCEEDED) {
                        _log_error("Failed to create transformed color image: " + std::to_string(status));
                        return;
                    }

                    status = k4a_transformation_color_image_to_depth_camera(transformation_handle,
                        depth_image,
                        color_image,
                        transformed_color_image
                    );

                    if (status != K4A_RESULT_SUCCEEDED) {
                        _log_error("Failed to compute transformed color image: " + std::to_string(status));
                        return;
                    }


                    k4a_image_release(color_image);
                    color_image = transformed_color_image;
                }
                uint8_t* data_pointer = k4a_image_get_buffer(color_image);
                const size_t size = k4a_image_get_size(color_image);
                int width = k4a_image_get_width_pixels(color_image);
                int height = k4a_image_get_height_pixels(color_image);
                int stride = k4a_image_get_stride_bytes(color_image);
                int format = k4a_image_get_format(color_image);

                std::string description =
                    "width=" + std::to_string(width) +
                    ",height=" + std::to_string(height) +
                    ",stride=" + std::to_string(stride) +
                    ",format=" + std::to_string(format);

                void* pointer = malloc(size);

                if (pointer) {
                    memcpy(pointer, data_pointer, size);
                    cwipc_metadata* ap = pc->access_metadata();
                    ap->_add(name, description, pointer, size, ::free);
                }

                k4a_image_release(color_image);
            }

            if (metadata.want_depth) {
                std::string name = "depth." + serial;
                if (!filtering.map_color_to_depth) {
                    k4a_image_t transformed_depth_image = NULL;
                    k4a_result_t status;
                    status = k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
                        color_image_width_pixels,
                        color_image_height_pixels,
                        color_image_width_pixels * (int)sizeof(uint16_t),
                        &transformed_depth_image
                    );

                    if (status != K4A_RESULT_SUCCEEDED) {
                        _log_error("Failed to create transformed depth image: " + std::to_string(status));
                        return;
                    }

                    status = k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image);
                    if (status != K4A_RESULT_SUCCEEDED) {
                        _log_error("Failed to compute transformed depth image: " + std::to_string(status));
                        return;
                    }
                    k4a_image_release(depth_image);
                    depth_image = transformed_depth_image;
                }

                uint8_t* data_pointer = k4a_image_get_buffer(depth_image);
                const size_t size = k4a_image_get_size(depth_image);
                int width = k4a_image_get_width_pixels(depth_image);
                int height = k4a_image_get_height_pixels(depth_image);
                int stride = k4a_image_get_stride_bytes(depth_image);
                int format = k4a_image_get_format(depth_image);

                std::string description =
                    "width=" + std::to_string(width) +
                    ",height=" + std::to_string(height) +
                    ",stride=" + std::to_string(stride) +
                    ",format=" + std::to_string(format);

                void* pointer = malloc(size);

                if (pointer) {
                    memcpy(pointer, data_pointer, size);
                    cwipc_metadata* ap = pc->access_metadata();
                    ap->_add(name, description, pointer, size, ::free);
                }
            }
        }

        if (metadata.want_skeleton) {
            _save_metadata_skeleton(pc);
        }

        if (metadata.want_camera_specs) {
            _save_metadata_camera_specs(pc);
        }
    }

protected:
    // internal API that is "shared" with other implementations (realsense, kinect)
    virtual bool _init_hardware_for_this_camera() = 0;
    // xxxjack _init_config_for_this_camera() has different signatures for camera/recording.
    virtual bool _init_filters() override final {
        // K4A API does not implement any filtering, so nothing to initialize
        // xxxjack or should we initialize the XY table here?
        return true;
    }

    virtual bool _init_skeleton_tracker() override final {
#ifdef CWIPC_WITH_KINECT_SKELETONS
        if (tracker_handle != NULL) {
            return true;
        }

        k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
        tracker_config.processing_mode = K4ABT_TRACKER_PROCESSING_MODE_CPU;

        if (skeleton.bt_processing_mode >= 0) {
            tracker_config.processing_mode = (k4abt_tracker_processing_mode_t)skeleton.bt_processing_mode;
        }

        if (skeleton.bt_sensor_orientation >= 0) {
            tracker_config.sensor_orientation = (k4abt_sensor_orientation_t)skeleton.bt_sensor_orientation;
        }

#ifdef K4ABT_SUPPORTS_MODEL_PATH
        if (skeleton.bt_model_path != "") {
            tracker_config.model_path = skeleton.bt_model_path.c_str();
        }
#endif

        auto sts = k4abt_tracker_create(&sensor_calibration, tracker_config, &tracker_handle);
        const char* processingModesStr[] = { "GPU", "CPU", "GPU_CUDA", "GPU_TENSORRT", "GPU_DIRECTML" };
        const char* sensorOrientationsStr[] = { "Default", "Clockwise_90", "CounterClockwise_90", "Flip_180" };

        if (sts != K4A_RESULT_SUCCEEDED) {
            _log_error("Body tracker initialization failed: " + std::to_string(sts));
            return false;
        } else {
            _log_trace(std::string("Body tracker initialized. Processing mode: ") +
                processingModesStr[tracker_config.processing_mode] +
                ", Sensor orientation: " + sensorOrientationsStr[tracker_config.sensor_orientation]
            );
        }

        return true;
#else
        return false;
#endif
    }

    virtual void _apply_filters() final {
        // xxxjack to be implemented. and used.
    }

    virtual void _apply_filters_to_depth_image(k4a_image_t depth_image) {
        if (filtering.do_threshold) {
            uint16_t* depth_buffer = (uint16_t*)(void*)k4a_image_get_buffer(depth_image);

            int width = k4a_image_get_width_pixels(depth_image);
            int height = k4a_image_get_height_pixels(depth_image);

            int16_t min_depth = (int16_t)(filtering.threshold_near * 1000);
            int16_t max_depth = (int16_t)(filtering.threshold_far * 1000);
            min_depth = (min_depth > 1) ? min_depth : 1; // min depth should be > minimum_operating_range (250-500 for kinect). 0 means no data.
            max_depth = (max_depth > min_depth) ? max_depth : 5460; // max depth should be >min_depth otherwise we use the maximum_operating_range (5460 for kinect).

            // First we create the depth material from the depth_data
            cv::Mat depth_in(height, width, CV_16UC1, depth_buffer);

            // DISTANCE FILTER: we create a mask to filter data using the distance thresholds
            cv::Mat mask = cv::Mat::ones(height, width, CV_8UC1);
            cv::inRange(depth_in, min_depth, max_depth, mask); //we check if the depth values are in the wanted range and create a mask

            // EROSION FILTER: if erosion wanted, we will erode the mask using a kernel.
            int x_delta = processing.depth_x_erosion;
            int y_delta = processing.depth_y_erosion;
            if (x_delta || y_delta) {
                cv::Mat kernel = cv::getStructuringElement(CV_16UC1, cv::Size(x_delta*2+1, y_delta*2+1), cv::Point(-1, -1)); //we want erosion on 4 directions. +-x, +-y.
                cv::erode(mask, mask, kernel, cv::Point(-1, -1), 1);
            }

            //APPLYING THE MASK:
            cv::Mat depth_out;
            depth_in.copyTo(depth_out, mask); //we apply the mask

            //copy filtered_depthmap data back to initial depth_buffer
            memcpy(depth_buffer, depth_out.data, depth_out.step * depth_out.rows);
        }
    }
    virtual void _start_capture_thread() = 0;
    virtual void _capture_thread_main() = 0;
    void _start_processing_thread() {
        camera_processing_thread = new std::thread(&K4ABaseCamera::_processing_thread_main, this);
        _cwipc_setThreadName(camera_processing_thread, L"cwipc_kinect::camera_processing_thread");
    }

    void _processing_thread_main() {
        if (debug) _log_debug_thread("frame processing thread started");
        while (!camera_stopped) {
            //
            // Get the frameset we need to turn into a point cloud
            ///
            k4a_capture_t processing_frameset = NULL;
            bool ok = processing_frame_queue.wait_dequeue_timed(processing_frameset, std::chrono::milliseconds(10000));
            if (end_of_stream_reached) break;
            if (!ok) {
                if (waiting_for_capture) _log_warning("processing thread dequeue timeout");
                std::this_thread::yield();
                continue;
            }
            waiting_for_capture = false;
            if (processing_frameset == nullptr) {
                if (!camera_stopped) _log_error("processing thread dequeue produced NULL pointer");
                break;
            }
            if (debug) _log_debug_thread("processing thread got frameset");
            assert(processing_frameset);
            
            std::lock_guard<std::mutex> lock(processing_mutex);
#ifdef CWIPC_WITH_KINECT_SKELETONS
            //
            // use body tracker for skeleton extraction
            //
            if (tracker_handle) {
                _feed_frameset_to_tracker(processing_frameset);
            }
#endif
            //
            // get depth and color images. Apply filters and uncompress color image if needed
            //
            k4a_image_t depth_image = k4a_capture_get_depth_image(processing_frameset);
            k4a_image_t color_image = k4a_capture_get_color_image(processing_frameset);
            //
            // Do processing on the images (filtering, decompressing)
            //
            _apply_filters_to_depth_image(depth_image); //filtering depthmap => better now because if we map depth to color then we need to filter more points.
            color_image = cwipc_k4a_uncompress_color_image(this, processing_frameset, color_image);
            //  
            // generate pointcloud
            //
            cwipc_pcl_pointcloud new_pointcloud = nullptr;
            if (filtering.map_color_to_depth) {
                new_pointcloud = _generate_point_cloud_color_to_depth(depth_image, color_image);
            } else {
                new_pointcloud = _generate_point_cloud_depth_to_color(depth_image, color_image);
            }

            if (new_pointcloud != nullptr) {
                current_pcl_pointcloud = new_pointcloud;
                if (debug) _log_debug_thread("generated pointcloud with " + std::to_string(current_pcl_pointcloud->size()) + " points");

                if (current_pcl_pointcloud->size() == 0) {
                    _log_warning("Captured empty pointcloud from camera");
                    //continue;
                }
                //
                // Notify wait_for_pointcloud_processed that we're done.
                //
                processing_done = true;
                processing_done_cv.notify_one();
            }
            //
            // Release resources no longer needed.
            //
            if (depth_image) {
                k4a_image_release(depth_image);
            }
            if (color_image) {
                k4a_image_release(color_image);
            }
            if (processing_frameset) {
                k4a_capture_release(processing_frameset);
            }
        }

        if (debug) _log_debug_thread("processing thread exiting");
    }

protected:
    // Auxiliary methods only applicable to Kinect implementation.
    void _feed_frameset_to_tracker(k4a_capture_t processing_frameset) {
        //
        // Push frameset into the tracker. Wait indefinitely for the result.
        //
        k4a_wait_result_t queue_capture_result = k4abt_tracker_enqueue_capture(tracker_handle, processing_frameset, K4A_WAIT_INFINITE);
        if (queue_capture_result == K4A_WAIT_RESULT_TIMEOUT) {
            // It should never hit timeout when K4A_WAIT_INFINITE is set.
            _log_warning("k4abt_tracker_enqueue_capture: timeout");
        } else if (queue_capture_result == K4A_WAIT_RESULT_FAILED) {
            _log_warning("k4abt_tracker_enqueue_capture: failed");
        }

        //
        // Now pop the result. Again wait indefinitely.
        //
        k4abt_frame_t body_frame = NULL;
        skeletons.clear();
        k4a_wait_result_t pop_frame_result = k4abt_tracker_pop_result(tracker_handle, &body_frame, K4A_WAIT_INFINITE);

        if (pop_frame_result == K4A_WAIT_RESULT_SUCCEEDED) {
            uint32_t num_bodies = k4abt_frame_get_num_bodies(body_frame);
            if (debug) _log_debug_thread("_feed_frameset_to_tracker: detected " + std::to_string(num_bodies) + " bodies");
            if (num_bodies > 0) {
                // Transform each 3d joints from 3d depth space to 2d color image space
                for (uint32_t i = 0; i < num_bodies; i++) {
                    //printf("- Person[%u]:\n", i);
                    k4abt_skeleton_t skeleton;
                    auto sts = k4abt_frame_get_body_skeleton(body_frame, i, &skeleton);

                    if (sts != K4A_RESULT_SUCCEEDED) {
                        _log_error("_feed_frameset_to_tracker: Get body from body frame failed");
                        break;
                    }

                    for (int joint_id = 0; joint_id < (int)K4ABT_JOINT_COUNT; joint_id++) {
                        k4a_float3_t::_xyz pos = skeleton.joints[joint_id].position.xyz; //millimiters
                        cwipc_pcl_point point;
                        point.x = pos.x;
                        point.y = pos.y;
                        point.z = pos.z;

                        if (!filtering.map_color_to_depth) {
                            _transform_point_depth_to_color(point);
                        }

                        _transform_point_cam_to_world(point);
                        pos.x = point.x;
                        pos.y = point.y;
                        pos.z = point.z;
                        skeleton.joints[joint_id].position.xyz = pos;
                    }

                    skeletons.push_back(skeleton);
                }
            }
        } else if (pop_frame_result == K4A_WAIT_RESULT_TIMEOUT) {
            _log_warning("k4abt_tracker_pop_result: timeout");
        } else {
            _log_warning("k4abt_tracker_pop_result: failed");
        }

        if (body_frame != nullptr) {
            k4abt_frame_release(body_frame);
        }
    }

    /// Kinect-specific: save skeleton metadata into point cloud.
    void _save_metadata_skeleton(cwipc_pointcloud* pc) {
#ifdef CWIPC_WITH_KINECT_SKELETONS
        int n_skeletons = skeletons.size();
        size_t size_str = sizeof(cwipc_skeleton_collection) + n_skeletons * (int)K4ABT_JOINT_COUNT * sizeof(cwipc_skeleton_joint);
        cwipc_skeleton_collection* skl = (cwipc_skeleton_collection*)malloc(size_str);

        if (skl != NULL) {
            skl->n_skeletons = n_skeletons;
            skl->n_joints = (int)K4ABT_JOINT_COUNT;
            cwipc_skeleton_joint* p = skl->joints;
            for (auto s : skeletons) {
                for (auto j : s.joints) {
                    p->confidence = (int)j.confidence_level;
                    p->x = j.position.xyz.x;
                    p->y = j.position.xyz.y;
                    p->z = j.position.xyz.z;
                    p->q_w = j.orientation.wxyz.w;
                    p->q_x = j.orientation.wxyz.x;
                    p->q_y = j.orientation.wxyz.y;
                    p->q_z = j.orientation.wxyz.z;
                    p++;
                }
            }

            std::string name = "skeleton." + serial;
            cwipc_metadata* ap = pc->access_metadata();
            ap->_add(name, "", (void*)skl, size_str, ::free);
        }
#endif
    }

    /// Kinect-specific: save camera specifics metadata.
    void _save_metadata_camera_specs(cwipc_pointcloud* pc) {

        // Get calibration (color and depth)
        const k4a_calibration_t& calibration = sensor_calibration;
        const k4a_calibration_camera_t& color_calibration = calibration.color_camera_calibration;
        const k4a_calibration_camera_t& depth_calibration = calibration.depth_camera_calibration;

        // Extract the required specs
        K4ACameraMetadataCameraSpecs* specs = (K4ACameraMetadataCameraSpecs*)malloc(sizeof(K4ACameraMetadataCameraSpecs));
        // K4ACaptureMetadataCameraSpecs* specs = new K4ACaptureMetadataCameraSpecs();
        specs->focal_length_x = color_calibration.intrinsics.parameters.param.fx;
        specs->focal_length_y = color_calibration.intrinsics.parameters.param.fy;
        specs->principal_point_x = color_calibration.intrinsics.parameters.param.cx;
        specs->principal_point_y = color_calibration.intrinsics.parameters.param.cy;
        specs->color_image_width = static_cast<unsigned int>(color_calibration.resolution_width);
        specs->color_image_height = static_cast<unsigned int>(color_calibration.resolution_height);
        // note: the official values from the API are given in millimeters
        // but the units of the 3D points are already converted to meters, 
        // so use meters here as well.
        specs->near_plane = 0.01f; // 10.0f = 10mm
        specs->far_plane = 10.0f; // 10000.0f = 10000mm

        // Save the specs in the meta-data
        const std::string name = "camera." + serial;
        cwipc_metadata* ap = pc->access_metadata();
        ap->_add(name, "", (void*)specs, sizeof(K4ACameraMetadataCameraSpecs), ::free);
    }

    //virtual void _computePointSize() = 0;

    /// Create a cwipc_pcl_pointcloud from depth and color frame, for color-to-depth mapping.
    virtual cwipc_pcl_pointcloud _generate_point_cloud_color_to_depth(const k4a_image_t depth_image, const k4a_image_t color_image) final {
        int depth_image_width_pixels = k4a_image_get_width_pixels(depth_image);
        int depth_image_height_pixels = k4a_image_get_height_pixels(depth_image);
        k4a_image_t transformed_color_image = NULL;
        k4a_result_t status;

        status = k4a_image_create(K4A_IMAGE_FORMAT_COLOR_BGRA32,
            depth_image_width_pixels,
            depth_image_height_pixels,
            depth_image_width_pixels * 4 * (int)sizeof(uint8_t),
            &transformed_color_image
        );

        if (status != K4A_RESULT_SUCCEEDED) {
          _log_error("Failed to create transformed color image: " + std::to_string(status));
          return nullptr;
        }

        status = k4a_transformation_color_image_to_depth_camera(transformation_handle,
            depth_image,
            color_image,
            transformed_color_image
        );

        if (status != K4A_RESULT_SUCCEEDED) {
            _log_error("Failed to compute transformed color image: " + std::to_string(status));
            return nullptr;
        }

        cwipc_pcl_pointcloud rv;


        rv = _generate_point_cloud_common(depth_image, transformed_color_image);

        k4a_image_release(transformed_color_image);

        return rv;
    }

    /// Create a cwipc_pcl_pointcloud from depth and color frame, for depth-to-color mapping.
    virtual cwipc_pcl_pointcloud _generate_point_cloud_depth_to_color(const k4a_image_t depth_image, const k4a_image_t color_image) final {
        // transform color image into depth camera geometry
        int color_image_width_pixels = k4a_image_get_width_pixels(color_image);
        int color_image_height_pixels = k4a_image_get_height_pixels(color_image);
        k4a_image_t transformed_depth_image = NULL;
        k4a_result_t status;

        status = k4a_image_create(K4A_IMAGE_FORMAT_DEPTH16,
            color_image_width_pixels,
            color_image_height_pixels,
            color_image_width_pixels * (int)sizeof(uint16_t),
            &transformed_depth_image
        );

        if (status != K4A_RESULT_SUCCEEDED) {
            _log_error("Failed to create transformed depth image: " + std::to_string(status));
            return nullptr;
        }

        status = k4a_transformation_depth_image_to_color_camera(transformation_handle, depth_image, transformed_depth_image);
        if (status != K4A_RESULT_SUCCEEDED) {
            _log_error("Failed to compute transformed depth image: " + std::to_string(status));
            return nullptr;
        }

        cwipc_pcl_pointcloud rv;

        rv = _generate_point_cloud_common(transformed_depth_image, color_image);

        k4a_image_release(transformed_depth_image);

        return rv;
    }

    /// Create a cwipc_pcl_pointcloud from depth and color frame, common code.
    virtual cwipc_pcl_pointcloud _generate_point_cloud_common(const k4a_image_t depth_image, k4a_image_t color_image) {
        int width = k4a_image_get_width_pixels(depth_image);
        int height = k4a_image_get_height_pixels(depth_image);

        float height_min = processing.height_min;
        float height_max = processing.height_max;
        bool do_height_filtering = height_min < height_max;
        bool do_greenscreen_removal = processing.greenscreen_removal;
        bool do_radius_filtering = processing.radius_filter > 0;

        //access depth and color data
        uint16_t* depth_data = (uint16_t*)(void*)k4a_image_get_buffer(depth_image);
        k4a_float2_t* uv_factors_for_depth_table = (k4a_float2_t*)(void*)k4a_image_get_buffer(depth_uv_mapping);
        cwi_bgra_t* color_data = (cwi_bgra_t*)(void*)k4a_image_get_buffer(color_image);

        cwipc_pcl_pointcloud new_cloud = new_cwipc_pcl_pointcloud();
        new_cloud->clear();
        new_cloud->reserve(width * height);

        for (int i = 0; i < width * height; i++) {
            if (depth_data[i] != 0 && !isnan(uv_factors_for_depth_table[i].xy.x) && !isnan(uv_factors_for_depth_table[i].xy.y)) {
                cwipc_pcl_point point;
                point.x = uv_factors_for_depth_table[i].xy.x * (float)depth_data[i];
                point.y = uv_factors_for_depth_table[i].xy.y * (float)depth_data[i];
                point.z = (float)depth_data[i];
                point.b = color_data[i].bgra.b;
                point.g = color_data[i].bgra.g;
                point.r = color_data[i].bgra.r;
                point.a = (uint8_t)1 << camera_index;

                if (filtering.map_color_to_depth) {
                    _transform_point_depth_to_color(point);
                }
                _transform_point_cam_to_world(point); //transforming from camera to world coordinates

                if (do_height_filtering && (point.y < height_min || point.y > height_max)) { //height filtering
                    continue;
                }

                if (do_radius_filtering && !isPointInRadius(point, processing.radius_filter)) {
                    continue;
                }

                if (processing.greenscreen_removal && !isNotGreen(&point)) { //chromakey removal
                    continue;
                }

                //point passed all filters, so we add this to the pointcloud
                new_cloud->push_back(point);
            }
        }
        if (debug) _log_debug_thread("produced " + std::to_string(new_cloud->size()) + " points");
        return new_cloud;
    }

    /// Create the depth to color UV mapping table. Doing this ourselves in
    /// stead of relying on the Kinect SDK turns out to be much faster.
    virtual void _create_depth_uv_mapping(const k4a_calibration_t* calibration) {
        int width;
        int height;

        if (filtering.map_color_to_depth) {
            width = calibration->depth_camera_calibration.resolution_width;
            height = calibration->depth_camera_calibration.resolution_height;
        } else {
            width = calibration->color_camera_calibration.resolution_width;
            height = calibration->color_camera_calibration.resolution_height;
        }

        k4a_result_t status;
        status = k4a_image_create(K4A_IMAGE_FORMAT_CUSTOM,
            width,
            height,
            width * (int)sizeof(k4a_float2_t),
            &depth_uv_mapping
        );

        if (status != K4A_RESULT_SUCCEEDED) {
            _log_error("Failed to create XY table image: " + std::to_string(status));
            return;
        }

        k4a_float2_t* table_data = (k4a_float2_t*)(void*)k4a_image_get_buffer(depth_uv_mapping);

        k4a_float2_t p;
        k4a_float3_t ray;
        int valid;

        for (int y = 0, idx = 0; y < height; y++) {
            p.xy.y = (float)y;

            for (int x = 0; x < width; x++, idx++) {
                p.xy.x = (float)x;

                if (filtering.map_color_to_depth) {
                    k4a_calibration_2d_to_3d(
                        calibration, &p, 1.f, K4A_CALIBRATION_TYPE_DEPTH, K4A_CALIBRATION_TYPE_DEPTH, &ray, &valid
                    );
                } else {
                    k4a_calibration_2d_to_3d(
                        calibration, &p, 1.f, K4A_CALIBRATION_TYPE_COLOR, K4A_CALIBRATION_TYPE_COLOR, &ray, &valid
                    );
                }

                if (valid) {
                    table_data[idx].xy.x = ray.xyz.x;
                    table_data[idx].xy.y = ray.xyz.y;
                } else {
                    table_data[idx].xy.x = nanf("");
                    table_data[idx].xy.y = nanf("");
                }
            }
        }
    }

    /// Convert a 3D point from camera coordinates to world coordinates.
    virtual void _transform_point_cam_to_world(cwipc_pcl_point& pt) final {
        float x = pt.x / 1000.0;
        float y = pt.y / 1000.0;
        float z = pt.z / 1000.0;
        pt.x = (*camera_config.trafo)(0,0)*x + (*camera_config.trafo)(0,1)*y + (*camera_config.trafo)(0,2)*z + (*camera_config.trafo)(0,3);
        pt.y = (*camera_config.trafo)(1,0)*x + (*camera_config.trafo)(1,1)*y + (*camera_config.trafo)(1,2)*z + (*camera_config.trafo)(1,3);
        pt.z = (*camera_config.trafo)(2,0)*x + (*camera_config.trafo)(2,1)*y + (*camera_config.trafo)(2,2)*z + (*camera_config.trafo)(2,3);
    }

    /// Convert a 3D point from depth camera coordinates to color camera coordinates.
    virtual void _transform_point_depth_to_color(cwipc_pcl_point& pt) final {
        float x = pt.x;
        float y = pt.y;
        float z = pt.z;
        float *rotation = depth_to_color_extrinsics.rotation;
        float *translation = depth_to_color_extrinsics.translation;
        pt.x = rotation[0] * x + rotation[1] * y + rotation[2] * z + translation[0];
        pt.y = rotation[3] * x + rotation[4] * y + rotation[5] * z + translation[1];
        pt.z = rotation[6] * x + rotation[7] * y + rotation[8] * z + translation[2];
    }
  
    /// Old method to create point cloud from point cloud image and color image.
    /// Kept for referrence, because the new method _generate_point_cloud is much faster.
    virtual cwipc_pcl_pointcloud _old_generate_point_cloud(const k4a_image_t point_cloud_image, const k4a_image_t color_image) final {
        int width = k4a_image_get_width_pixels(point_cloud_image);
        int height = k4a_image_get_height_pixels(color_image);
        bool do_height_filtering = processing.height_min != 0 || processing.height_max != 0;
        uint8_t * color_data = k4a_image_get_buffer(color_image);
        int16_t* point_cloud_image_data = (int16_t*)k4a_image_get_buffer(point_cloud_image);

        // now loop over images and create points.
        cwipc_pcl_pointcloud new_cloud = new_cwipc_pcl_pointcloud();
        new_cloud->clear();
        new_cloud->reserve(width * height);

        for (int i = 0; i < width * height; i++) {
            int i_pc = i * 3;
            int i_rgba = i * 4;
            cwipc_pcl_point point;
            int16_t x = point_cloud_image_data[i_pc + 0];
            int16_t y = point_cloud_image_data[i_pc + 1];
            int16_t z = point_cloud_image_data[i_pc + 2];

            if (z == 0) {
                continue;
            }

            // color_data is BGR
            point.r = color_data[i_rgba + 2];
            point.g = color_data[i_rgba + 1];
            point.b = color_data[i_rgba + 0];

            if (configuration.single_tile >= 0) {
                point.a = configuration.single_tile;
            } else {
                point.a = (uint8_t)1 << camera_index;
            }
            uint8_t alpha = color_data[i_rgba + 3];

            if (point.r == 0 && point.g == 0 && point.b == 0 && alpha == 0) {
                continue;
            }

            point.x = x;
            point.y = y;
            point.z = z;

            if (filtering.map_color_to_depth) {
              _transform_point_depth_to_color(point);
            }

            _transform_point_cam_to_world(point);

            if (processing.radius_filter > 0.0) { // apply radius filter
                if(!isPointInRadius(point, processing.radius_filter)) {
                    continue;
                }
            }

            if (do_height_filtering && (point.y < processing.height_min || point.y > processing.height_max)) {
                continue;
            }

            if (!processing.greenscreen_removal || isNotGreen(&point)) {// chromakey removal
                new_cloud->push_back(point);
            }
        }
        if (debug) _log_debug_thread("produced " + std::to_string(new_cloud->size()) + " points");
        return new_cloud;
    }
public:
    float pointSize = 0;  //<! (Approximate) 3D cellsize of pointclouds captured by this camera
    std::string serial; //<! Serial number for this camera
    bool end_of_stream_reached = false; //<! True when end of file reached on this camera stream
    int camera_index;

protected:
    K4ACaptureConfig& configuration;
    K4ACameraConfig& camera_config; //<! Per-camera data for this camera
    K4ACaptureProcessingConfig& processing;
    K4ACameraProcessingParameters& filtering;
    K4ACameraHardwareConfig& hardware;
    K4ASkeletonConfig& skeleton;
    K4ACaptureMetadataConfig& metadata; //<! Metadata configuration

    Type_api_camera camera_handle;
    bool camera_stopped = true;  //<! True when stopping
    bool camera_started = false;  //<! True when camera hardware is grabbing
    std::thread* camera_processing_thread = nullptr; //<! Handle for thread that runs processing loop
    std::thread* camera_capturer_thread = nullptr;  //<! Handle for thread that rungs grabber (if applicable)
#ifdef CWIPC_WITH_KINECT_SKELETONS
    std::vector<k4abt_skeleton_t> skeletons; //<! Skeletons extracted using the body tracking sdk
#endif
    cwipc_pcl_pointcloud current_pcl_pointcloud = nullptr;  //<! Most recent grabbed pointcloud
    k4a_transformation_t transformation_handle = nullptr; //<! k4a structure describing relationship between RGB and D cameras
    moodycamel::BlockingReaderWriterQueue<k4a_capture_t> captured_frame_queue;  //<! Frames from capture-thread, waiting to be inter-camera synchronized
    moodycamel::BlockingReaderWriterQueue<k4a_capture_t> processing_frame_queue;  //<! Synchronized frames, waiting for processing thread
    k4a_capture_t current_captured_frameset = nullptr; //<! Current frame being moved from captured_frame_queue to processing_frame_queue
    bool waiting_for_capture = false;
    bool camera_sync_ismaster;  //<! Parameter from camData xxxjack needs to go
    bool camera_sync_inuse; //<! Parameter from camData xxxjack needs to go
    bool debug = false;
    std::mutex processing_mutex;  //<! Exclusive lock for frame to pointcloud processing.
    std::condition_variable processing_done_cv; //<! Condition variable signalling pointcloud ready
    bool processing_done = false; //<! Boolean for processing_done_cv
#ifdef CWIPC_WITH_KINECT_SKELETONS
    k4abt_tracker_t tracker_handle = nullptr; //<! Handle to k4abt skeleton tracker
#endif
    k4a_calibration_t sensor_calibration; //<! k4a calibration data read from hardware camera or recording
    k4a_calibration_extrinsics_t depth_to_color_extrinsics; //<! k4a calibration data read from hardware camera or recording
    k4a_image_t depth_uv_mapping = NULL;
    std::string record_to_file; //<! If non-empty: file to record the captured streams to.
};
