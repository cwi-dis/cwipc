#pragma once

#include <string>
#include <mutex>
#include <condition_variable>

#include "libobsensor/h/ObTypes.h"
#include "libobsensor/hpp/Frame.hpp"
#include "libobsensor/hpp/Pipeline.hpp"
#include "libobsensor/hpp/Utils.hpp"
#include "readerwriterqueue.h"

#include "cwipc_util/internal/capturers.hpp"
#include "OrbbecConfig.hpp"
template<typename Type_api_camera> 
class OrbbecBaseCamera : public CwipcBaseCamera {


public:
    OrbbecBaseCamera(const std::string& _Classname, Type_api_camera _handle, OrbbecCaptureConfig& _configuration, OrbbecCaptureMetadataConfig& _metadata, int _camera_index)
    :   CwipcBaseCamera(_Classname + ": " + _configuration.all_camera_configs[_camera_index].serial, "orbbec"),
        camera_index(_camera_index),
        configuration(_configuration),
        serial(_configuration.all_camera_configs[_camera_index].serial),
        camera_sync_ismaster(serial == configuration.sync.sync_master_serial),
        camera_stopped(true),
        camera_config(_configuration.all_camera_configs[_camera_index]),
        filtering(_configuration.filtering),
        processing(_configuration.processing),
        hardware(_configuration.hardware),
        metadata(_metadata),
        camera_device(_handle),
        captured_frame_queue(1),
        processing_frame_queue(1),
        camera_sync_inuse(configuration.sync.sync_master_serial != ""),
        current_captured_frameset(nullptr),
        debug(_configuration.debug)    
    {
    }

    virtual ~OrbbecBaseCamera() {
        camera_device = nullptr;
        camera_pipeline = nullptr;
    }

    /// Step 1 in starting: tell the camera we are going to start. Called for all cameras.
    virtual bool pre_start_all_cameras() final { 
        if (!_init_filters()) {
            return false;
        }
        if (!_init_hardware_for_this_camera()) {
            return false;
        }
        return true;
    }
    /// Step 2 in starting: starts the camera. Called for all cameras. 
    virtual bool start_camera() override = 0;
    /// Step 3 in starting: starts the capturer. Called after all cameras have been started.
    virtual void start_camera_streaming() override = 0;
    /// Step 4, called after all capturers have been started.
    virtual void post_start_all_cameras() override final {

    }
    virtual void pre_stop_camera() override final {
      
    }

    virtual void stop_camera() final {
        if (debug) _log_debug("stop camera");
        camera_stopped = true;
        // clear out processing_frame_queue...
        while(true) {
            std::shared_ptr<ob::FrameSet> dummy;
            if (!processing_frame_queue.try_dequeue(dummy)) break;
        }
        // push nullptr to ensure wakeup of processing_thread
        processing_frame_queue.try_enqueue(nullptr);
        // join it.
        if (camera_processing_thread) {
            camera_processing_thread->join();
        }
        delete camera_processing_thread;
        camera_processing_thread = nullptr;

        if (camera_started && camera_pipeline != nullptr) {
            camera_pipeline->stop();
            camera_pipeline = nullptr;
            camera_started = false;
        }
        processing_done = true;
        processing_done_cv.notify_one();
        if (debug) _log_debug("camera stopped");
    }

    virtual bool is_sync_master() override final {
        return camera_sync_ismaster;
    }

    virtual bool mapcolordepth(int x_c, int y_c, int* out2d) override final {
        // Orbbec (like Kinect) uses the same coordinates for color and depth images.
        out2d[0] = x_c;
        out2d[1] = y_c;
        return true;
    }

    virtual bool map2d3d(int x_2d, int y_2d, int d_2d, float* out3d) override final {
        if (current_processed_frameset == nullptr) {
            _log_error("map2d3d: current_processed_frameset is NULL");
            return false;
        }
        std::shared_ptr<ob::Frame> depth_frame = current_processed_frameset->getFrame(OB_FRAME_DEPTH);
        if (depth_frame == nullptr) {
            _log_error("map2d3d: missing depth frame");
            return false;
        }
        std::shared_ptr<ob::StreamProfile> depth_profile = depth_frame->getStreamProfile();
        if (depth_profile == nullptr) {
            _log_error("map2d3d: missing depth profile");
            return false;
        }
        std::shared_ptr<ob::VideoStreamProfile> depth_video_profile = depth_profile->as<ob::VideoStreamProfile>();
        if (depth_video_profile == nullptr) {
            _log_error("map2d3d: depth profile wrong type");
            return false;
        }
        OBCameraIntrinsic depth_intrinsic = depth_video_profile->getIntrinsic();
        OBExtrinsic depth_extrinsic = depth_video_profile->getExtrinsicTo(depth_video_profile);
        OBPoint2f point {(float)x_2d, (float)y_2d};
        float depth = d_2d;
        OBPoint3f result;
        ob_error *error;
        bool ok = ob_transformation_2d_to_3d(point, depth, depth_intrinsic, depth_extrinsic, &result, &error);
        if (!ok) {
            _log_error(std::string("map2d3d: ob_error: ")+ob_error_get_message(error));
            return false;
        }
        cwipc_pcl_point pt = {
            (float)(result.x/1000.0),
            (float)(result.y/1000.0),
            (float)(result.z/1000.0),
            0, 0, 0, 0};
        _log_debug("map2d3d: result x="+std::to_string(pt.x) + ",y="+std::to_string(pt.y)+",z="+std::to_string(pt.z));
        _transform_point_cam_to_world(pt);
        out3d[0] = pt.x;
        out3d[1] = pt.y;
        out3d[2] = pt.z;
        return true;
    }

    /// Get current camera hardware parameters.
    /// xxxjack to be overridden for real cameras.
    virtual void get_camera_hardware_parameters(OrbbecCameraHardwareConfig& output) {
        output = hardware;
    }

    /// Return true if current hardware parameters of this camera match input.
    bool match_camera_hardware_parameters(OrbbecCameraHardwareConfig& input) {
        return (
            false
        );
    }

public:
    /// Step 1 in capturing: wait for a valid frameset. Any image processing will have been done. 
    /// Returns timestamp of depth frame, or zero if none available.
    /// This ensures protected attribute current_captured_frameset is valid.

    virtual uint64_t wait_for_captured_frameset(uint64_t minimum_timestamp) final {
        if (camera_stopped || camera_pipeline == nullptr) return 0;
        uint64_t resultant_timestamp = 0;
        do {
            waiting_for_capture = true;
            current_captured_frameset = camera_pipeline->waitForFrameset();
            if (current_captured_frameset == nullptr) return 0;
            std::shared_ptr<ob::Frame> depth_frame = current_captured_frameset->getFrame(OB_FRAME_DEPTH);
            if (depth_frame == nullptr) {
            _log_warning("frameset without depth frame: " + std::to_string(current_captured_frameset->getIndex()));
            return 1; // xxxjack is this a good idea?
            }
            resultant_timestamp = depth_frame->getTimeStampUs();
            if (resultant_timestamp < minimum_timestamp) {
            _log_trace("drop frame with dts=" + std::to_string(resultant_timestamp));
            }
        } while (resultant_timestamp < minimum_timestamp);
        if (debug) _log_debug("wait_for_captured_frameset: dts=" + std::to_string(resultant_timestamp));
        return resultant_timestamp;
    }
    /// Step 2: Forward the current_captured_frameset to the processing thread to turn it into a point cloud.
    virtual void process_pointcloud_from_frameset() final {
        assert(current_captured_frameset && current_captured_frameset.getImpl());

        if (!processing_frame_queue.try_enqueue(current_captured_frameset)) {
            _log_warning("processing_frame_queue full, dropping frame");
            // Orbbec releases the current_captured_frameset automatically.
        }
        current_captured_frameset = nullptr;
    }
    /// Step 3: Wait for the point cloud processing.
    /// After this, current_pcl_pointcloud and current_processed_frameset will be valid.
    virtual void wait_for_pointcloud_processed() final {
        std::unique_lock<std::mutex> lock(processing_mutex);
        processing_done_cv.wait(lock, [this] { return processing_done; });
        processing_done = false;
    }
    /// Step 4: borrow a pointer to the point cloud just created, as a PCL point cloud.
    /// The capturer will use this to populate the resultant cwipc point cloud with points
    /// from all cameras.
    cwipc_pcl_pointcloud access_current_pcl_pointcloud() { return current_pcl_pointcloud; }
    /// Step 5: Save metadata from frameset into given cwipc object.
    void save_frameset_metadata(cwipc_pointcloud *pc) {
        auto current_frameset = current_captured_frameset;
        if (current_frameset == nullptr) {
            _log_error("save_frameset_metadata: current_frameset is NULL");
            return;
        }
        if (metadata.want_depth || metadata.want_rgb) {
            std::shared_ptr<ob::Frame> depth_frame = current_frameset->getFrame(OB_FRAME_DEPTH);
            std::shared_ptr<ob::Frame> color_frame = current_frameset->getFrame(OB_FRAME_COLOR);
            if (depth_frame == nullptr || color_frame == nullptr) {
                _log_error("save_frameset_metadata: Failed to get depth frame or color frame from capture for metadata");
                return;
            }
            OBFormat depth_format = depth_frame->format();
            OBFormat color_format = color_frame->format();
            if (depth_format != OB_FORMAT_Y16) {
                _log_error("save_frameset_metadata: Depth frame is not OB_FORMAT_Y16: " + std::to_string(depth_frame->format()));
                return;
            }
            if (color_format != OB_FORMAT_BGRA) {
                _log_error("save_frameset_metadata: Color frame is not OB_FORMAT_BGRA: " + std::to_string(color_frame->format()));
                return;
            }
            std::shared_ptr<ob::DepthFrame> depth_image = depth_frame->as<ob::DepthFrame>();
            std::shared_ptr<ob::ColorFrame> color_image = color_frame->as<ob::ColorFrame>();
            int color_image_width_pixels = color_image->getWidth();
            int color_image_height_pixels = color_image->getHeight();
            int depth_image_width_pixels = depth_image->getWidth();
            int depth_image_height_pixels = depth_image->getHeight();
            if (metadata.want_rgb) {
                std::string name = "rgb." + serial;
#if 0
                color_image = _uncompress_color_image(current_processed_frameset, color_image);
#endif
                int bpp=4;
                int stride = color_image_width_pixels*bpp;
                size_t size = color_image_height_pixels * color_image_width_pixels * bpp;
                std::string description = 
                    "width="+std::to_string(color_image_width_pixels)+
                    ",height="+std::to_string(color_image_height_pixels)+
                    ",stride="+std::to_string(stride)+
                    ",bpp="+std::to_string(bpp)+
                    ",format="+"BGRA";
                void *pointer = malloc(size);
                if (pointer) {
                    memcpy(pointer, color_image->getData(), size);
                    cwipc_metadata* ap = pc->access_metadata();
                    ap->_add(name, description, pointer, size, ::free);
                }
            }
            if (metadata.want_depth) {
                std::string name = "depth." + serial;
#if 0
                // xxxjack map_color_to_depth??
#endif
                int bpp=2;
                int stride = depth_image_width_pixels*bpp;
                size_t size = depth_image_height_pixels * depth_image_width_pixels * bpp;
                std::string description = 
                    "width="+std::to_string(depth_image_width_pixels)+
                    ",height="+std::to_string(depth_image_height_pixels)+
                    ",stride="+std::to_string(stride)+
                    ",bpp="+std::to_string(bpp)+
                    ",format="+"Z16";
                void *pointer = malloc(size);
                if (pointer) {
                    memcpy(pointer, depth_image->getData(), size);
                    cwipc_metadata* ap = pc->access_metadata();
                    ap->_add(name, description, pointer, size, ::free);
                }
            }
        }
    }

protected:
    // internal API that is "shared" with other implementations (realsense, kinect)
    /// Initialize any hardware settings for this camera.
    virtual bool _init_hardware_for_this_camera() override = 0;
    /// Initialize any filters that will be applied to all RGB/D images.
    virtual bool _init_filters() override final {
        // Orbbec API does not provide any filtering that needs to be initialized.
        return true;
    }
    /// Apply filter to a frameset.
    virtual bool _init_skeleton_tracker() override final { 
        // Body tracker not supported for Orbbec
        return true; 
    }

    virtual void _apply_filters() final {
        // xxxjack to be implemented, maybe
    }
    virtual void _start_capture_thread() = 0;
    virtual void _capture_thread_main() = 0;
    void _start_processing_thread() {
        camera_processing_thread = new std::thread(&OrbbecBaseCamera::_processing_thread_main, this);
        _cwipc_setThreadName(camera_processing_thread, L"cwipc_orbbec::camera_processing_thread");
    }

    void _processing_thread_main() {
        if(debug) _log_debug_thread("frame processing thread started");
        while (!camera_stopped) {
            if (debug) _log_debug_thread("11. wait for processing_frame_queue");
            //
            // Get the frameset we need to turn into a point cloud
            ///
            std::shared_ptr<ob::FrameSet> processing_frameset;
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
            if (debug) _log_debug_thread("12. Lock processing_mutex");
            std::lock_guard<std::mutex> lock(processing_mutex);
#if 0
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
            std::shared_ptr<ob::Frame> depth_frame = processing_frameset->getFrame(OB_FRAME_DEPTH);
            if (depth_frame == nullptr) {
                _log_warning("empty point cloud, missing depth frame in frameset " + std::to_string(processing_frameset->getIndex()));
                current_pcl_pointcloud = new_cwipc_pcl_pointcloud();
                processing_done = true;
                processing_done_cv.notify_one();
                continue;
            }
            std::shared_ptr<ob::DepthFrame> depth_image = depth_frame->as<ob::DepthFrame>();
            std::shared_ptr<ob::Frame> color_frame = processing_frameset->getFrame(OB_FRAME_COLOR);
            if (color_frame == nullptr) {
                _log_warning("missing color frame in frameset " + std::to_string(processing_frameset->getIndex()));
                current_pcl_pointcloud = new_cwipc_pcl_pointcloud();
                processing_done = true;
                processing_done_cv.notify_one();
                continue;
            }
            std::shared_ptr<ob::ColorFrame> color_image = color_frame->as<ob::ColorFrame>();
            if (debug) _log_debug(std::string("Processing frame:") +
                    " depth: " + std::to_string(depth_frame->getIndex()) +":" + std::to_string(depth_image->getWidth()) + "x" + std::to_string(depth_image->getHeight()) +
                    " color: " + std::to_string(color_frame->getIndex()) +":"  + std::to_string(color_image->getWidth()) + "x" + std::to_string(color_image->getHeight()));
            //
            // Do processing on the images (filtering, decompressing)
            //
#if 0
            // xxxjack to do
            _apply_filters_to_depth_image(depth_image); //filtering depthmap => better now because if we map depth to color then we need to filter more points.
            color_image = _uncompress_color_image(processing_frameset, color_image);
#endif
            // Keep frameset for metadata, map2d3d, etc.
            current_processed_frameset = processing_frameset;

            //  
            // generate pointcloud
            //
            if (debug) _log_debug_thread("13. _generate_pointcloud()");
            cwipc_pcl_pointcloud new_pointcloud = nullptr;
            new_pointcloud = _generate_point_cloud(processing_frameset);

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
                if (debug) _log_debug_thread("14. notified processing_done_cv");
            } else {
                _log_warning("_generate_point_cloud() returned NULL");
            }
            //
            // No cleanup needed, orbbec API handles it.
            //
        }
        if (camera_started && camera_pipeline != nullptr) {
            camera_pipeline->stop();
            camera_pipeline = nullptr;
            camera_started = false;
        }
        if (debug)_log_debug_thread("processing thread exiting");
    }

    cwipc_pcl_pointcloud _generate_point_cloud(std::shared_ptr<ob::FrameSet> frameset) {
        cwipc_pcl_pointcloud pcl_pointcloud = new_cwipc_pcl_pointcloud();
        auto pointcloud_filter = std::make_shared<ob::PointCloudFilter>();
        pointcloud_filter->setCreatePointFormat(OB_FORMAT_RGB_POINT);
        std::shared_ptr<ob::Frame> pointcloud_frame = pointcloud_filter->process(frameset);
        std::shared_ptr<ob::PointsFrame> points_frame = pointcloud_frame->as<ob::PointsFrame>();
        auto format = points_frame->getFormat();
        if (format != OB_FORMAT_RGB_POINT) {
            _log_warning("_generate_point_cloud: format is not OB_FORMAT_RGB_POINT");
            return pcl_pointcloud;
        }
        uint32_t width  = points_frame->getWidth();
        uint32_t height = points_frame->getHeight();

        pcl_pointcloud->reserve(width*height);
        OBColorPoint* points = reinterpret_cast<OBColorPoint *>(points_frame->getData());
        float height_min = processing.height_min;
        float height_max = processing.height_max;
        bool do_height_filtering = height_min < height_max;
        bool do_greenscreen_removal = processing.greenscreen_removal;
        bool do_radius_filtering = processing.radius_filter > 0;
        for (int idx = 0; idx < width*height; idx++) {
            OBColorPoint *obpt = points + idx;
            cwipc_pcl_point pt(obpt->x / 1000.0, obpt->y / 1000.0, obpt->z / 1000.0);
            if (pt.z == 0) continue;
            _transform_point_cam_to_world(pt);
            if (do_height_filtering && (pt.y < height_min || pt.y > height_max)) {
                continue;
            }
            if (do_radius_filtering && !isPointInRadius(pt, processing.radius_filter)) {
                continue;
            }
            // xxxjack NOTE: the color names in OBColorPoint seem to be mixed up.
            pt.r = (uint8_t)(obpt->b);
            pt.g = (uint8_t)(obpt->g);
            pt.b = (uint8_t)(obpt->r);
            pt.a = (uint8_t)(1 << camera_index);
            if (do_greenscreen_removal && !isNotGreen(&pt)) {
                continue;
            }
            pcl_pointcloud->push_back(pt);
        }
        return pcl_pointcloud;
    }

    void _transform_point_cam_to_world(cwipc_pcl_point& pt) {
    float x = (*camera_config.trafo)(0,0)*pt.x + (*camera_config.trafo)(0,1)*pt.y + (*camera_config.trafo)(0,2)*pt.z + (*camera_config.trafo)(0,3);
    float y = (*camera_config.trafo)(1,0)*pt.x + (*camera_config.trafo)(1,1)*pt.y + (*camera_config.trafo)(1,2)*pt.z + (*camera_config.trafo)(1,3);
    float z = (*camera_config.trafo)(2,0)*pt.x + (*camera_config.trafo)(2,1)*pt.y + (*camera_config.trafo)(2,2)*pt.z + (*camera_config.trafo)(2,3);
    pt.x = x;
    pt.y = y;
    pt.z = z;
}

public:
    float pointSize = 0;
    std::string serial;
    bool end_of_stream_reached = false;
    int camera_index;

protected:
    OrbbecCaptureConfig configuration;
    OrbbecCameraConfig& camera_config;
    OrbbecCaptureProcessingConfig& processing;
    OrbbecCameraProcessingParameters& filtering;
    OrbbecCameraHardwareConfig& hardware;
    OrbbecCaptureMetadataConfig& metadata; //<! Metadata configuration
    
    Type_api_camera camera_device = nullptr;
    std::shared_ptr<ob::Pipeline> camera_pipeline = nullptr;
    bool camera_started = false;
    bool camera_stopped = true;
    std::thread *camera_capturer_thread;
    std::thread* camera_processing_thread = nullptr; //<! Handle for thread that runs processing loop
    cwipc_pcl_pointcloud current_pcl_pointcloud = nullptr;  //<! Most recent grabbed pointcloud

    moodycamel::BlockingReaderWriterQueue<std::shared_ptr<ob::FrameSet>> captured_frame_queue;
    moodycamel::BlockingReaderWriterQueue<std::shared_ptr<ob::FrameSet>> processing_frame_queue;
    std::shared_ptr<ob::FrameSet> current_captured_frameset;
    std::shared_ptr<ob::FrameSet> current_processed_frameset;
    bool waiting_for_capture = false;           //< Boolean to stop issuing warning messages while paused.
    bool camera_sync_ismaster;
    bool camera_sync_inuse;
    std::mutex processing_mutex;  //<! Exclusive lock for frame to pointcloud processing.
    std::condition_variable processing_done_cv; //<! Condition variable signalling pointcloud ready
    bool processing_done = false;
    bool debug = false;
    std::string record_to_file;
    bool uses_recorder = false;

};
