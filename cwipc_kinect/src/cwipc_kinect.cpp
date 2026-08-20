
#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_KINECT_EXPORT __declspec(dllexport)
#endif

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/capturers.hpp"
#include "cwipc_kinect/api.h"
#include "K4AConfig.hpp"
#include "K4ACapture.hpp"
#include "K4APlaybackCapture.hpp"
#define stringify(x) _stringify(x)
#define _stringify(x) #x


static bool _api_versioncheck(char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        char* msgbuf = (char*)malloc(1024);
        snprintf(msgbuf, 1024, "cwipc_kinect: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
        if (errorMessage) {
            *errorMessage = msgbuf;
        }
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_kinect", msgbuf + 13);
        return false;
    }
    return true;
}

/** Base class for Kinect capturer implementations
 * 
 * Used for both live Kinect and Kinect playback, implements functionality 
 * common to both types of capturers.
 */
template<class GrabberClass, class CameraConfigClass=K4ACameraConfig>
class cwipc_source_kinect_impl_base : public cwipc_capturer_impl_base<GrabberClass, CameraConfigClass> {
public:
    using cwipc_capturer_impl_base<GrabberClass, CameraConfigClass>::cwipc_capturer_impl_base;
    
    void request_metadata(const std::string &name) override {
        cwipc_activesource::request_metadata(name);

        this->m_grabber->request_metadata(
            cwipc_activesource::is_metadata_requested("rgb"),
            cwipc_activesource::is_metadata_requested("depth"),
            false,
            cwipc_activesource::is_metadata_requested("skeleton"),
            cwipc_activesource::is_metadata_requested("camera")
        );
    }

    bool auxiliary_operation(const std::string op, const void* inbuf, size_t insize, void* outbuf, size_t outsize) override {
        // For test purposes, really...
        if (op == "map2d3d") {
            if (inbuf == nullptr || insize != 4 * sizeof(float)) return false;
            if (outbuf == nullptr || outsize != 3 * sizeof(float)) return false;
            float* infloat = (float*)inbuf;
            float* outfloat = (float*)outbuf;
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

    virtual bool seek(uint64_t timestamp) = 0;

};

/** Implementation of Kinect capturer for live Kinect devices */
class cwipc_source_kinect_impl : public cwipc_source_kinect_impl_base<K4ACapture> {

public:
    using cwipc_source_kinect_impl_base<K4ACapture>::cwipc_source_kinect_impl_base;
    bool seek(uint64_t timestamp) override {
        return false;
    }
};

/** Implementation of Kinect capturer for playback */
class cwipc_source_kinect_playback_impl : public cwipc_source_kinect_impl_base<K4APlaybackCapture> {
public:
    using cwipc_source_kinect_impl_base<K4APlaybackCapture>::cwipc_source_kinect_impl_base;
    bool seek(uint64_t timestamp) override {
        if (m_grabber == NULL) {
            return NULL;
        }

        bool rv = m_grabber->seek(timestamp);
        return rv;
    }
};

//
// C-compatible entry points
//

const char *cwipc_get_version_kinect() {
#ifdef CWIPC_VERSION
    return stringify(CWIPC_VERSION);
#else
    return "unknown";
#endif
}


cwipc_activesource* cwipc_kinect(const char *configFilename, char **errorMessage, uint64_t apiVersion) {
    if (! _api_versioncheck(errorMessage,  apiVersion)) {
        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_source_kinect_impl *rv = new cwipc_source_kinect_impl(configFilename);

    // If the grabber found cameras everything is fine
    if (rv && rv->can_start()) {
        cwipc_log_set_errorbuf(nullptr);
        return rv;
    }

    delete rv;
    cwipc_log_set_errorbuf(nullptr);
    if (errorMessage && *errorMessage == NULL) {
        *errorMessage = (char *)"cwipc_kinect: unspecified error";
    }

    return NULL;
}

cwipc_activesource* cwipc_kinect_playback(const char* configFilename, char** errorMessage, uint64_t apiVersion) {
    if (! _api_versioncheck(errorMessage,  apiVersion)) {
        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_source_kinect_playback_impl* rv = new cwipc_source_kinect_playback_impl(configFilename);

    // If the grabber found cameras everything is fine
    if (rv && rv->can_start()) {
        cwipc_log_set_errorbuf(nullptr);
        return rv;
    }

    delete rv;
    cwipc_log_set_errorbuf(nullptr);

    if (errorMessage && *errorMessage == NULL) {
        *errorMessage = (char *)"cwipc_kinect_playback: unspecified error";
    }
    return NULL;
}

//
// These static variables only exist to ensure the initializer is called, which registers our camera type.
//
int _cwipc_dummy_kinect_initializer = _cwipc_register_capturer("kinect", K4ACapture::count_devices, cwipc_kinect);
int _cwipc_dummy_kinect_playback_initializer = _cwipc_register_capturer("kinect_playback", nullptr, cwipc_kinect_playback);
// For backward compatibility
int _cwipc_dummy_kinect_offline_initializer = _cwipc_register_capturer("kinect_offline", nullptr, cwipc_kinect_playback);
