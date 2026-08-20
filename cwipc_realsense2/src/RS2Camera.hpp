#ifndef cwipc_realsense_RS2Camera_hpp
#define cwipc_realsense_RS2Camera_hpp
#pragma once

#include "RS2BaseCamera.hpp"

class RS2Camera : public RS2BaseCamera {
public:
    using RS2BaseCamera::RS2BaseCamera;
    
    virtual void post_start_all_cameras() override final;
    virtual bool seek(uint64_t timestamp) override final { return false; }
    virtual bool eof() override final { return false;}
protected:
    virtual void _init_pipeline_for_this_camera(rs2::config &cfg) override final;
    virtual void _post_start_this_camera(rs2::pipeline_profile& profile) override final;
};
#endif // cwipc_realsense_RS2Camera_hpp
