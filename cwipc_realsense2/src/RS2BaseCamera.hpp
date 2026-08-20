#ifndef cwipc_realsense_RS2BaseCamera_hpp
#define cwipc_realsense_RS2BaseCamera_hpp
#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>

#include <librealsense2/rs.hpp>

#include "cwipc_util/internal/capturers.hpp"

#include "RS2Config.hpp"

class RS2BaseCamera : public CwipcBaseCamera {
public:
    // The public API is for use by the Capturer
    RS2BaseCamera(rs2::context& _ctx, RS2CaptureConfig& configuration, RS2CaptureMetadataConfig& metadata, int _camera_index);
    virtual ~RS2BaseCamera();
    /// Step 1 in starting: tell the camera we are going to start. Called for all cameras.
    virtual bool pre_start_all_cameras() override final;
    /// Step 2 in starting: starts the camera. Called for all cameras. 
    virtual bool start_camera() override final;
    /// Step 3 in starting: starts the capturer. Called after all cameras have been started.
    virtual void start_camera_streaming() override;
    /// Step 4, called after all capturers have been started.
    virtual void post_start_all_cameras() override = 0;

    /// Prepare for stopping the cameras. May do something like stopping the recording.
    virtual void pre_stop_camera() override final;
    /// Completely stops camera and capturer, releases all resources. Can be re-started with start_camera, etc.
    virtual void stop_camera()  override final;
    /// Return true if this camera is the sync master.
    virtual bool is_sync_master() override final {
        return camera_sync_is_master;
    }
    /// Seek. Fails for cameras, overriden for playback cameras.
    virtual bool seek(uint64_t timestamp) override = 0;
    /// Are we at end-of-file? Always false for cameras.
    virtual bool eof() override = 0;

    /// Implementation of mapping 2D color image coordinates to 2D depth image coordinates.
    virtual bool mapcolordepth(int x_c, int y_c, int *out2d) override final;
    /// Implementation of mapping 2D color image coordinates to 3D coordinates.
    virtual bool map2d3d(int x_2d, int y_2d, int d_2d, float* out3d) override final;

    /// Get current camera hardware parameters.
    /// xxxjack may have to become virtual, and only really do something
    /// for real cameras. Or just return current settings in base class (here).
    void get_camera_hardware_parameters(RS2CameraHardwareConfig& output);
    /// Return true if current hardware parameters of this camera match input.
    bool match_camera_hardware_parameters(RS2CameraHardwareConfig& input);

public:

    /// Step 1 in capturing: wait for a valid frameset. Any image processing will have been done. 
    /// Returns timestamp of depth frame, or zero if none available.
    /// This ensures protected attribute current_captured_frameset is valid.
    uint64_t wait_for_captured_frameset(uint64_t minimum_timestamp);
    /// Step 2: Forward the current_captured_frameset to the processing thread to turn it into a point cloud.
    void process_pointcloud_from_frameset();
    /// Step 3: Wait for the point cloud processing.
    /// After this, current_pcl_pointcloud and current_processed_frameset will be valid.
    void wait_for_pointcloud_processed();
    /// Step 4: borrow a pointer to the point cloud just created, as a PCL point cloud.
    /// The capturer will use this to populate the resultant cwipc point cloud with points
    /// from all cameras.
    cwipc_pcl_pointcloud access_current_pcl_pointcloud() { return current_pcl_pointcloud; }
    /// Step 5: Save metadata from frameset into given cwipc object.
    void save_frameset_metadata(cwipc_pointcloud *pc);
protected:
    // internal API that is "shared" with other implementations (realsense, kinect)
    virtual bool _init_hardware_for_this_camera() override final { 
        // In the librealsense API hardware initialization is done globally,
        // in RS2Capture::_init_hardware_for_all_cameras
        return true; 
    }
    /// Realsense-specific: Prepare the rs2::config structure.
    /// This may enable the recorder, it may enable the correct streams that we want. 
    virtual void _init_pipeline_for_this_camera(rs2::config& cfg) = 0;
    virtual bool _init_filters() override final;
    virtual bool _init_skeleton_tracker() override final {
        // librealsense does not provide body tracking.
        return false;
    }
    virtual void _apply_filters_to_frameset(rs2::frameset& frames);
    virtual void _apply_rs2_filters(rs2::frameset& frames);
    void _start_capture_thread() {}
    void _capture_thread_main() {}
    void _start_processing_thread();
    void _processing_thread_main();

protected:
    /// Create a cwipc_pcl_pointcloud from depth and color frame.
    cwipc_pcl_pointcloud _generate_point_cloud(rs2::depth_frame depth, rs2::video_frame color);
    /// Called after this camera has started. Obtains actual hardware parameters and
    /// stores those in the configuration structures.
    /// Camera extends it to ensure recordings are as synchroniszed as possible.
    /// PlaybackCamera extends it to ensure playback is as synchronized as possible.
    virtual void _post_start_this_camera(rs2::pipeline_profile& profile);
    /// Small wrapper around librealsense method, to print debug information
    virtual rs2::frameset _wait_for_frames_from_pipeline();

    /// Erosion. Note that Kinect implementation has a better one, based on OpenCV.
    void _erode_depth(rs2::depth_frame, int x_delta, int y_delta);

    /// Compute reasonable pointsize, by looking at the distance between adjacent
    /// depth-image points at (3d-world) location (0, 0, 0)
    void _computePointSize(rs2::pipeline_profile profile);
    
    /// Transform a 3D point in camera-origin coordinates to world coordinates.
    void _transform_point_cam_to_world(cwipc_pcl_point& pt);
    

public:
    const int frame_timeout_ms = 1000;
    float pointSize; ///< Needed by RS2Capture, computed once at camera start
    bool end_of_stream_reached = false; //<! True when end of file reached on this camera stream

    int camera_index; ///< Needed by RS2Capture
    std::string serial; ///< Needed by RS2Capture

protected:
    RS2CameraConfig& camera_config;
    RS2CaptureProcessingConfig& processing;
    RS2CameraProcessingParameters& filtering;
    RS2CameraHardwareConfig& hardware;
    RS2CaptureMetadataConfig& metadata;
    std::string record_to_file;
    bool uses_recorder = false;
    bool camera_sync_is_master;
    bool camera_stopped;
    bool camera_started;

    std::thread *camera_processing_thread;
    
    rs2::pipeline camera_pipeline;
    // Realsense doesn't need captured_frame_queue, the library handles this.
    rs2::frame_queue processing_frame_queue;  //<! Synchronized frames, waiting for processing thread
    std::mutex processing_mutex;
    std::condition_variable processing_done_cv;
    bool processing_done;
    
    rs2::frameset previous_captured_frameset;
    rs2::frameset current_captured_frameset;    //< Output of capturer, input to processing
    bool waiting_for_capture = false;           //< Boolean to stop issuing warning messages while paused.
    rs2::frameset current_processed_frameset;   //< Output of processing
    cwipc_pcl_pointcloud current_pcl_pointcloud;    //< Output of processing

    // for an explanation of filtering see librealsense/doc/post-processing-filters.md and code in librealsense/src/proc
    rs2::align rs2filter_align_color_to_depth;                 // Align depth and color data
    rs2::align rs2filter_align_depth_to_color;                   // Align depth and color data
    rs2::decimation_filter rs2filter_decimation;                        // Decimation - reduces depth frame density
    // rs2::hdr_merge not supported yet.
    // rs2::sequence_id_filter not supported yet
    rs2::threshold_filter rs2filter_threshold;                   // Thresholding: minimum and maximum distance
    rs2::disparity_transform rs2filter_depth_to_disparity = rs2::disparity_transform(true);
    rs2::spatial_filter rs2filter_spatial;                          // Spatial    - edge-preserving spatial smoothing
    rs2::temporal_filter rs2filter_temporal;                         // Temporal   - reduces temporal noise
    rs2::hole_filling_filter rs2filter_hole_filling;
    rs2::disparity_transform rs2filter_disparity_to_depth = rs2::disparity_transform(false);
#ifdef cwipc_global_pointcloud_filter
    rs2::pointcloud rs2filter_depth_to_pointcloud;     // The pointcloud constructor
#endif

    bool debug = false;
    bool prefer_color_timing = true;    // If we get a second frame with the same depth timestamp (but newer color frame) we skip the old one.
    const char *depth_format = "unknown";
    const char *color_format = "unknown";
};

#endif