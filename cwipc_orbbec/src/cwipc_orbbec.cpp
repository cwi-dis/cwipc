
#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_ORBBEC_EXPORT __declspec(dllexport)
#endif

#include <inttypes.h>

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/capturers.hpp"
#include "cwipc_orbbec/api.h"

#include "OrbbecConfig.hpp"
#include "OrbbecCapture.hpp"
#include "OrbbecPlaybackCapture.hpp"
#include "OrbbecCamera.hpp"
#include "OrbbecPlaybackCamera.hpp"
#define stringify(x) _stringify(x)
#define _stringify(x) #x


static bool _api_versioncheck(char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        char* msgbuf = (char*)malloc(1024);
        snprintf(msgbuf, 1024, "cwipc_orbbec: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
        if (errorMessage) {
            *errorMessage = msgbuf;
        }
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_orbbec", msgbuf + 18);
        return false;
    }
    return true;
}

template<class GrabberClass, class CameraConfigClass=OrbbecCameraConfig>
class cwipc_source_orbbec_impl_base : public cwipc_capturer_impl_base<GrabberClass, CameraConfigClass> {

public:
    using cwipc_capturer_impl_base<GrabberClass, CameraConfigClass>::cwipc_capturer_impl_base;

    virtual void request_metadata(const std::string &name) override final {
        cwipc_activesource::request_metadata(name);
        this->m_grabber->request_metadata(
            cwipc_activesource::is_metadata_requested("rgb"), 
            cwipc_activesource::is_metadata_requested("depth"), 
            false,
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

class cwipc_source_orbbec_impl : public cwipc_source_orbbec_impl_base<OrbbecCapture> {
public:
    using cwipc_source_orbbec_impl_base<OrbbecCapture>::cwipc_source_orbbec_impl_base;

    bool seek(uint64_t timestamp) override {
        return false;
    }
};
class cwipc_source_orbbec_playback_impl : public cwipc_source_orbbec_impl_base<OrbbecPlaybackCapture> {
public:
    using cwipc_source_orbbec_impl_base<OrbbecPlaybackCapture>::cwipc_source_orbbec_impl_base;

    bool seek(uint64_t timestamp) override {
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

const char *cwipc_get_version_orbbec() {
#ifdef CWIPC_VERSION
    return stringify(CWIPC_VERSION);
#else
    return "unknown";
#endif
}

cwipc_activesource* cwipc_orbbec(const char *configFilename, char **errorMessage, uint64_t apiVersion) {
    if (!_api_versioncheck(errorMessage, apiVersion)) {
        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_source_orbbec_impl *rv = new cwipc_source_orbbec_impl(configFilename);
    if (rv && rv->can_start()) {
        cwipc_log_set_errorbuf(nullptr);
        return rv;
    }
    delete rv;
    cwipc_log_set_errorbuf(nullptr);
    if (errorMessage && *errorMessage == NULL) {
        *errorMessage = (char *)"cwipc_orbbec: unspecified error";
    }

    return NULL;
}

cwipc_activesource* cwipc_orbbec_playback(const char* configFilename, char** errorMessage, uint64_t apiVersion) {

    if (!_api_versioncheck(errorMessage, apiVersion)) {
        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_source_orbbec_playback_impl* rv = new cwipc_source_orbbec_playback_impl(configFilename);
    if (rv && rv->can_start()) {
        cwipc_log_set_errorbuf(nullptr);
        return rv;
    }
    delete rv;
    cwipc_log_set_errorbuf(nullptr);
    if (errorMessage && *errorMessage == nullptr) {
        static char *tmp = (char *)"cwipc_orbbec_playback: unspecified error";
        *errorMessage = tmp;
    }

    return NULL;
}

//
// These static variables only exist to ensure the initializer is called, which registers our camera type.
//
int _cwipc_dummy_orbbec_initializer = _cwipc_register_capturer("orbbec", OrbbecCapture::countDevices, cwipc_orbbec);
int _cwipc_dummy_orbbec_playback_initializer = _cwipc_register_capturer("orbbec_playback", OrbbecPlaybackCapture::countDevices, cwipc_orbbec_playback);
