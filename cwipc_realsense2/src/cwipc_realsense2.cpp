#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_REALSENSE2_EXPORT __declspec(dllexport)
#endif
#include <inttypes.h>

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_realsense2/api.h"

#include "RS2Config.hpp"
#include "RS2Capture.hpp"
#include "RS2Camera.hpp"
#include "RS2PlaybackCapture.hpp"
#include "RS2PlaybackCamera.hpp"
#include <string>
#define stringify(x) _stringify(x)
#define _stringify(x) #x

// Global variables (constants, really)

_CWIPC_REALSENSE2_EXPORT int CWIPC_RS2_FORMAT_Z16 = RS2_FORMAT_Z16;
_CWIPC_REALSENSE2_EXPORT int CWIPC_RS2_FORMAT_RGB8 = RS2_FORMAT_RGB8;

static bool _rs2_versioncheck(char **errorMessage) {
    int version = rs2_get_api_version(nullptr);
    if ((version/100) == (RS2_API_VERSION/100)) {
        return true;
    }
    cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", "Built against librealsense " + std::to_string(RS2_API_VERSION) + " but " + std::to_string(version) + " is installed.");
    if (errorMessage) {
        static char errorBuf[80];
        snprintf(errorBuf, sizeof(errorBuf), "Built against librealsense %d but %d is installed.", RS2_API_VERSION, version);
        *errorMessage = errorBuf;
    }

    return false;
}

static bool _api_versioncheck(char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        char* msgbuf = (char*)malloc(1024);
        snprintf(msgbuf, 1024, "cwipc_realsense2: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
        if (errorMessage) {
            *errorMessage = msgbuf;
        }
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_realsense2", msgbuf + 18);
        return false;
    }
    return true;
}

/** Base class for RealSense2 capturer implementations
 * 
 * Used for both live RealSense2 and RealSense2 playback, implements functionality
 * common to both types of capturers.
 */
template<class GrabberClass, class CameraConfigClass=RS2CameraConfig>
class cwipc_source_realsense2_impl_base : public cwipc_capturer_impl_base<GrabberClass, CameraConfigClass>  {
public:
    using cwipc_capturer_impl_base<GrabberClass, CameraConfigClass>::cwipc_capturer_impl_base;
    
    virtual void request_metadata(const std::string& name) override final {
        cwipc_activesource::request_metadata(name);
        this->m_grabber->request_metadata(
            cwipc_activesource::is_metadata_requested("rgb"), 
            cwipc_activesource::is_metadata_requested("depth"), 
            cwipc_activesource::is_metadata_requested("timestamps"),
            false,
            cwipc_activesource::is_metadata_requested("camera")
        );
    }

    virtual bool auxiliary_operation(const std::string op, const void* inbuf, size_t insize, void* outbuf, size_t outsize) override final {
        if (op == "map2d3d") {
            if (inbuf == nullptr || insize != 4*sizeof(float)) return false;
            if (outbuf == nullptr || outsize != 3*sizeof(float)) return false;
            float *infloat = (float *)inbuf;
            float *outfloat = (float *)outbuf;
            int tilenum = (int)infloat[0];
            int x_2d = (int)infloat[1];
            int y_2d = (int)infloat[2];
            float d_2d = infloat[3];

            return this->m_grabber->map2d3d(tilenum, x_2d, y_2d, d_2d, outfloat);
        } else if (op == "mapcolordepth") {
            if (inbuf == nullptr || insize != 3*sizeof(int)) return false;
            if (outbuf == nullptr || outsize != 2*sizeof(int)) return false;
            int *inint = (int *)inbuf;
            int *outint = (int *)outbuf;

            return this->m_grabber->mapcolordepth(inint[0], inint[1], inint[2], outint);

        } else {
            return false;
        }
    }

    virtual bool seek(uint64_t timestamp) override = 0;
};

/** Implementation of RealSense2 capturer for live RealSense2 devices */
class cwipc_source_realsense2_impl : public cwipc_source_realsense2_impl_base<RS2Capture> {

public:
    using cwipc_source_realsense2_impl_base<RS2Capture>::cwipc_source_realsense2_impl_base;

    bool seek(uint64_t timestamp) override final {
        return false;
    }
};

/** Implementation of RealSense2 capturer for playback */
class cwipc_source_realsense2_playback_impl : public cwipc_source_realsense2_impl_base<RS2PlaybackCapture> {
public:
    using cwipc_source_realsense2_impl_base<RS2PlaybackCapture>::cwipc_source_realsense2_impl_base;

    bool seek(uint64_t timestamp) override final{
        if (m_grabber == NULL) {
            return false;
        }

        bool rv = m_grabber->seek(timestamp);
        return rv;
    }
};

//
// C-compatible entry points
//


const char *cwipc_get_version_realsense2() {
#ifdef CWIPC_VERSION
    return stringify(CWIPC_VERSION);
#else
    return "unknown";
#endif
}

cwipc_activesource* cwipc_realsense2(const char *configFilename, char **errorMessage, uint64_t apiVersion) {
    if (!_api_versioncheck(errorMessage, apiVersion)) {
        return NULL;
    }

    if (!_rs2_versioncheck(errorMessage)) {
        return NULL;
    }

    cwipc_log_set_errorbuf(errorMessage);
    cwipc_source_realsense2_impl *rv = new cwipc_source_realsense2_impl(configFilename);

    // If the grabber found cameras everything is fine
    if (rv && rv->can_start()) {
        cwipc_log_set_errorbuf(nullptr);
        return rv;
    }

    delete rv;
    cwipc_log_set_errorbuf(nullptr);
    if (errorMessage && *errorMessage == NULL) {
        *errorMessage = (char *)"cwipc_realsense2: unspecified error";
    }

    return NULL;
}


cwipc_activesource* cwipc_realsense2_playback(const char *configFilename, char **errorMessage, uint64_t apiVersion) {
    if (!_api_versioncheck(errorMessage, apiVersion)) {
        return NULL;
    }
    if (!_rs2_versioncheck(errorMessage)) {
        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_source_realsense2_playback_impl *rv = new cwipc_source_realsense2_playback_impl(configFilename);

    // If the grabber found cameras everything is fine
    if (rv && rv->can_start()) {
        cwipc_log_set_errorbuf(nullptr);
        return rv;
    }

    delete rv;
    cwipc_log_set_errorbuf(nullptr);
    if (errorMessage && *errorMessage == NULL) {
        *errorMessage = (char *)"cwipc_realsense2_playback: unspecified error";
    }

    return NULL;
}


//
// These static variables only exist to ensure the initializer is called, which registers our camera type.
//
int _cwipc_dummy_realsense_initializer = _cwipc_register_capturer("realsense", RS2Capture::count_devices, cwipc_realsense2);
int _cwipc_dummy_realsense_playback_initializer = _cwipc_register_capturer("realsense_playback", RS2PlaybackCapture::count_devices, cwipc_realsense2_playback);
