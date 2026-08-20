// cwi_encode.cpp : Defines the exported functions for the DLL application.
//
#include <cstdint>
#include <chrono>
#include <sstream>
#include <thread>
#include <condition_variable>
#include <inttypes.h>

#ifdef WIN32
#define _CWIPC_CODEC_EXPORT __declspec(dllexport)
#else
#define _CWIPC_CODEC_EXPORT
#endif

#undef SET_THREAD_NAME

#include "cwipc_codec_config.h"
#include "cwipc_util/api_pcl.h"
#include "cwipc_util/internal/logging.hpp"
#include "cwipc_codec/api.h"
#include "readerwriterqueue.h"
#include <pcl/point_cloud.h>
#include <pcl/cloud_codec_v2/point_cloud_codec_v2.h>

#define stringify(x) _stringify(x)
#define _stringify(x) #x

//
// Define the codec we want to use. Main options are probably using OctreeBase or Octree2BufBase.
//
#ifdef CWIPC_CODEC_WITH_SINGLE_BUF
typedef pcl::octree::OctreeBase<pcl::octree::OctreeContainerPointIndices,pcl::octree::OctreeContainerEmpty> cwipc_octree_type;
#else
typedef pcl::octree::Octree2BufBase<pcl::octree::OctreeContainerPointIndices,pcl::octree::OctreeContainerEmpty> cwipc_octree_type;
#endif

typedef pcl::io::OctreePointCloudCodecV2<
    cwipc_pcl_point,
    OctreeContainerPointIndices,
    OctreeContainerEmpty,
    cwipc_octree_type
> cwipc_pointcloud_codec;

// Some parameters that will eventually be customizable again
const int num_threads = 1;
const bool downdownsampling = false;
const int iframerate = 1;

class cwipc_encoder_impl : public cwipc_encoder {
public:
    cwipc_encoder_impl(cwipc_encoder_params *_params)
    :
        m_encoder(NULL),
        m_params(*_params),
        m_result(NULL),
        m_result_size(0),
        m_remaining_frames_in_gop(0),
        m_alive(true),
        m_queue_tilefilter(1),
        m_queue_encoder(1),
        m_thread_tilefilter(nullptr),
        m_thread_encoder(nullptr),
        m_use_threads(false)
    {}

    ~cwipc_encoder_impl() {}

    void free() {
        close();

        if (m_result) {
            ::free(m_result);
        }

        m_encoder = NULL;
        m_result = NULL;
        m_result_size = 0;
        m_result_cv.notify_all();
    }

    void close() {
        m_alive = false;
        m_result_cv.notify_all();

        if (m_use_threads) {
            m_queue_encoder.try_enqueue(nullptr);
            m_queue_tilefilter.try_enqueue(nullptr);

            m_thread_encoder->join();
            m_thread_tilefilter->join();

            delete m_thread_encoder;
            delete m_thread_tilefilter;

            m_thread_encoder = nullptr;
            m_thread_tilefilter = nullptr;

            m_use_threads = false;
        }
    }

    void enable_threads() {
        if (m_use_threads) {
            return;
        }

        m_use_threads = true;
        m_thread_encoder = new std::thread([this] {
#ifdef SET_THREAD_NAME
            std::string name = "pc-encoder tile=" + std::to_string(this->m_params.tilenumber) + " depth=" + std::to_string(this->m_params.octree_bits);
            pthread_setname_np(name.c_str());
#endif
            while (m_alive) {
                bool ok = this->_run_single_encode();
                if (!ok) {
                    if (m_alive) {
                        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encoder", "threaded encoder failure");
                    }
                    break;
                }
            }
            m_alive = false;
            m_result_cv.notify_all();
        });

        m_thread_tilefilter = new std::thread([this] {
#ifdef SET_THREAD_NAME
            std::string name = "pc-tilefilter tile=" + std::to_string(this->m_params.tilenumber) + " depth=" + std::to_string(this->m_params.octree_bits);
            pthread_setname_np(name.c_str());
#endif
            while (m_alive) {
                bool ok = this->_run_single_tilefilter();
                if (!ok) {
                    if (m_alive) {
                        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encoder", "threaded tilefilter failure");
                    }
                    break;
                }   
            }
            m_alive = false;
            m_queue_encoder.try_enqueue(nullptr);
        });
    }

    bool eof() {
        return !m_alive;
    }

    bool available(bool wait) {
        std::unique_lock<std::mutex> lock(m_result_mutex);

        if (wait) {
            m_result_cv.wait(lock, [this] {
                return m_result != NULL || !m_alive;
            });
        }

        return m_result != NULL;
    }

    bool at_gop_boundary() {
        return m_remaining_frames_in_gop <= 0;
    }

    void feed(cwipc_pointcloud *pc) {
        if (!m_alive) {
            cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_encoder", "feed() called after close()");
            return;
        }

        if (m_use_threads) {

            m_queue_tilefilter.enqueue(pc->_shallowcopy());
        } else {
            bool ok;

            ok = m_queue_tilefilter.try_enqueue(pc->_shallowcopy());
            if (!ok) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encoder", "unthreaded try_enqueue failed");
            }

            ok = _run_single_tilefilter();
            if (!ok) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encoder", "unthreaded tilefilter failure");
            }

            ok = _run_single_encode();
            if (!ok) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encoder", "unthreaded encode failure");
            }
        }
    }

    size_t get_encoded_size() {
      // xxxjack note that this is not thread-safe: before the copy_data()
      // call another thread could have grabbed the data.
      return m_result_size;
    }

    bool copy_data(void *buffer, size_t bufferSize) {
        std::lock_guard<std::mutex> lock(m_result_mutex);

        if (m_result == NULL || bufferSize < m_result_size) {
            return false;
        }

        memcpy(buffer, m_result, m_result_size);
        ::free(m_result);

        m_result = NULL;
        m_result_size = 0;

        return true;
    }

private:
    void alloc_encoder() {
        // xxxjack note that feed() and alloc_encoder() are not thread-safe.
        double point_resolution = std::pow ( 2.0, -1.0 * m_params.octree_bits);
        double octree_resolution = std::pow ( 2.0, -1.0 * m_params.octree_bits);

        m_encoder = NULL;
        m_encoder = pcl::shared_ptr<cwipc_pointcloud_codec > (
            new cwipc_pointcloud_codec (
                pcl::io::MANUAL_CONFIGURATION,
                false,
                point_resolution,
                octree_resolution,
                downdownsampling, // no intra voxel coding in this first version of the codec
                iframerate, // i_frame_rate,
                true, // do color encoding
                8, // color bits
                1, // color coding type
                false, // centroid computation - expensive with little added value
                false, // scalable bitstream - not implemented
                false, // do_connectivity_coding_ not implemented
                m_params.jpeg_quality,
                num_threads
            )
        );

        m_encoder->setMacroblockSize(m_params.macroblock_size);
        m_remaining_frames_in_gop = m_params.gop_size;
    }

    bool _run_single_tilefilter() {
        cwipc_pointcloud *pc = nullptr;
        m_queue_tilefilter.wait_dequeue(pc);

        if (pc == nullptr) {
            return false;
        }

        cwipc_pointcloud *newpc = nullptr;

        // Apply tile filtering, if needed
        if (m_params.tilenumber) {
            newpc = cwipc_tilefilter(pc, m_params.tilenumber);
            pc->free();
            if (newpc == NULL) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encoder", "tilefilter failed tile=" + std::to_string(m_params.tilenumber));
                return false;
            }
        } else {
            newpc = pc;
        }
        pc = nullptr; // Ensure we don't access this anymore.
        m_queue_encoder.enqueue(newpc);

        return true;
    }

    bool _run_single_encode() {
        cwipc_pointcloud *newpc = nullptr;
        m_queue_encoder.wait_dequeue(newpc);

        if (newpc == nullptr) {
            return false;
        }

        std::stringstream comp_frame;
        // Allocate an encoder if none is available (we are at the beginning of a GOP)
        if (m_encoder == NULL) {
            alloc_encoder();
        }

        bool start_new_gop = m_remaining_frames_in_gop <= 0;
        m_remaining_frames_in_gop = m_params.do_inter_frame ? m_params.gop_size : 1;

        cwipc_pcl_pointcloud pcl_pc = newpc->access_pcl_pointcloud();
        if (pcl_pc == nullptr) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encoder", "Pointcloud has NULL cwipc_pcl_pointcloud");
            return false;
        }
        if (pcl_pc->size() == 0) {
            // Special case: if the point cloud is empty we compress a point cloud with a single black point at 0,0,0
            pcl_pc = new_cwipc_pcl_pointcloud();
            cwipc_pcl_point dummyPoint;
            pcl_pc->push_back(dummyPoint);
        }

        // Note by Jack: this is a hack. If the point granularity of the pointcloud
        // is larger (more coarse) than the octree_resolution in the encoder we adapt
        // the encoder parameter, so a reasonable value is transmitted in the output
        // file or packet.
        float cellsize = newpc->cellsize();
        if (cellsize > m_encoder->octreeResolution) {
            m_encoder->octreeResolution = cellsize;
        }

        m_encoder->encodePointCloud(pcl_pc, comp_frame, newpc->timestamp());
        m_remaining_frames_in_gop--;

#ifdef delete_encoder_at_end_of_gop
        /* Free the encoder if we are at the end of the GOP */
        if (m_remaining_frames_in_gop <= 0) {
            m_encoder = NULL;
        }
#endif
        /* Store the result */
        std::lock_guard<std::mutex> lock(m_result_mutex);

        if (m_result) {
            ::free(m_result);
            m_result = NULL;
            m_result_size = 0;
        }

        m_result_size = comp_frame.str().length();
        m_result = (void *)malloc(m_result_size);
        comp_frame.str().copy((char *)m_result, m_result_size);
        m_result_cv.notify_one();
        newpc->free();

        return true;
    }

    pcl::shared_ptr<cwipc_pointcloud_codec > m_encoder;
    cwipc_encoder_params m_params;
    void *m_result;
    size_t m_result_size;
    std::mutex m_result_mutex;
    std::condition_variable m_result_cv;
    int m_remaining_frames_in_gop;
    bool m_alive;
    moodycamel::BlockingReaderWriterQueue<cwipc_pointcloud *> m_queue_tilefilter;
    moodycamel::BlockingReaderWriterQueue<cwipc_pointcloud *> m_queue_encoder;
    std::thread* m_thread_tilefilter;
    std::thread* m_thread_encoder;
    bool m_use_threads;
};

class cwipc_multithreaded_encoder_impl : public cwipc_encoder {
private:
    std::vector<cwipc_encoder_impl*> m_encoders;
    int m_nencoder;
    std::mutex m_next_in_mutex;
    int m_next_in;
    std::mutex m_next_out_mutex;
    int m_next_out;

public:
    cwipc_multithreaded_encoder_impl(cwipc_encoder_params *params)
    :
        m_nencoder(params->n_parallel),
        m_next_in(0),
        m_next_out(0)
    {
        for(int i=0; i<m_nencoder; i++) {
            auto e = new cwipc_encoder_impl(params);
            e->enable_threads();
            m_encoders.push_back(e);
        }
    }

    ~cwipc_multithreaded_encoder_impl() {}

    void free() {
        for (auto enc: m_encoders) {
            enc->free();
        }

        m_encoders.clear();
        m_nencoder = 0;
    }

    void close() {
        for (auto enc : m_encoders) enc->close();
    }

    void feed(cwipc_pointcloud *pc) {
        std::lock_guard<std::mutex> lock(m_next_in_mutex);
        m_encoders[m_next_in]->feed(pc);
        m_next_in = (m_next_in+1) % m_nencoder;
    }

    bool eof() {
        std::lock_guard<std::mutex> lock(m_next_out_mutex);

        if (m_nencoder == 0) {
            return true;
        }

        return m_encoders[m_next_out]->eof();
    }

    bool available(bool wait) {
        std::lock_guard<std::mutex> lock(m_next_out_mutex);

        if (m_nencoder == 0) {
            return false;
        }

        return m_encoders[m_next_out]->available(wait);
    }

    bool at_gop_boundary() {
        std::lock_guard<std::mutex> lock(m_next_out_mutex);

        if (m_nencoder == 0) {
            return true;
        }

        return m_encoders[m_next_out]->at_gop_boundary();
    }

    size_t get_encoded_size() {
        std::lock_guard<std::mutex> lock(m_next_out_mutex);

        if (m_nencoder == 0) {
            return 0;
        }

        return m_encoders[m_next_out]->get_encoded_size();
    }

    bool copy_data(void *buffer, size_t bufferSize) {
        std::lock_guard<std::mutex> lock(m_next_out_mutex);

        if (m_nencoder == 0) {
            return false;
        }

        int cur = m_next_out;
        m_next_out = (m_next_out+1) % m_nencoder;

        return m_encoders[cur]->copy_data(buffer, bufferSize);
    }
};

class cwipc_encodergroup_impl : public cwipc_encodergroup {
public:
    cwipc_encodergroup_impl() : m_voxelsize(-1) {}
    ~cwipc_encodergroup_impl() {}

    void free() {
        for (auto enc: m_encoders) {
            enc->free();
        }

        m_encoders.clear();
    }

    void close() {
        for (auto enc : m_encoders) {
            enc->close();
        }
    }

    void feed(cwipc_pointcloud *pc) {
        cwipc_pointcloud *newpc = NULL;

        if (m_voxelsize > 0) {
            newpc = cwipc_downsample(pc, m_voxelsize);

            if (newpc == NULL) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encodergroup", "downsampling failed");
                return;
            }
        } else {
            // Make shallow clone of pc
            newpc = pc->_shallowcopy();
        }

        pc = nullptr; // Ensure we don't access this anymore.

        for (auto enc : m_encoders) {
            enc->feed(newpc);
        }
        newpc->free();
    }

    cwipc_encoder *addencoder(int version, cwipc_encoder_params* params, char **errorMessage) {
        cwipc_log_set_errorbuf(errorMessage);
        if (m_voxelsize >= 0 && m_voxelsize != params->voxelsize) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encodergroup", "all encoders in a group must have the same voxelsize");
            cwipc_log_set_errorbuf(NULL);
            return NULL;
        }

        m_voxelsize = params->voxelsize;
        cwipc_encoder_impl *newEncoder = (cwipc_encoder_impl *)cwipc_new_encoder(version, params, errorMessage, CWIPC_API_VERSION);

        if (newEncoder == NULL) {
            if (errorMessage && *errorMessage == NULL) {
                cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_encodergroup::addencoder", "unspecified error");
            }
            cwipc_log_set_errorbuf(NULL);
            return NULL;
        }

        //
        // See if we should enable threading in the encoders.
        // If there was only a single one earlier we should enable threading for it too.
        //
        if (m_encoders.size() == 1) {
            m_encoders[0]->enable_threads();
        }

        if (m_encoders.size() > 0) {
            newEncoder->enable_threads();
        }

        m_encoders.push_back(newEncoder);
        cwipc_log_set_errorbuf(NULL);
        return newEncoder;
    }

private:
    std::vector<cwipc_encoder_impl *> m_encoders;
    float m_voxelsize;
};

class cwipc_decoder_impl : public cwipc_decoder {
public:
    cwipc_decoder_impl() : m_result(NULL), m_decoder_V2_(nullptr), m_alive(true) {}
    ~cwipc_decoder_impl() {}

    virtual void free() override final {
        m_alive = false;
        m_result_cv.notify_all();
    }

    virtual void close() override final {
        m_alive = false;
    }

    virtual bool seek(uint64_t timestamp) override final {
        return false;
    }

    virtual bool eof() override final {
        return !m_alive;
    }

    virtual bool available(bool wait) override final {
        std::unique_lock<std::mutex> lock(m_result_mutex);

        if (wait) {
            m_result_cv.wait(lock, [this]{
                return m_result != NULL || !m_alive;
            });
        }

        return m_result != NULL;
    }

    virtual void feed(void *buffer, size_t bufferSize) override final {
        if (!m_alive) {
            cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_decoder", "feed() called after close()");
            return;
        }

        if (m_decoder_V2_ == nullptr) {
            alloc_decoder();
        }

        cwipc_pcl_pointcloud decpc = new_cwipc_pcl_pointcloud();
        std::string str((char *)buffer, bufferSize);
        std::stringstream istream(str);
        uint64_t timeStamp = 0;
        bool ok = m_decoder_V2_->decodePointCloud(istream, decpc, timeStamp);

        if (decpc->size() == 1) {
            // Special case: single point (0,0,0,0,0,0) signals an empty pointcloud
            cwipc_pcl_point& pt(decpc->at(0));

            if (abs(pt.x) < 0.01 && abs(pt.y) < 0.01 && abs(pt.z) < 0.01 && pt.r < 2 && pt.g < 2 && pt.b < 2) {
                decpc->clear();
            }
        }

        std::lock_guard<std::mutex> lock(m_result_mutex);

        if (ok) {
            m_result = cwipc_from_pcl(decpc, timeStamp, NULL, CWIPC_API_VERSION);
            m_result->_set_cellsize(m_decoder_V2_->octreeResolution);
        } else {
            m_result = NULL;
        }

        m_result_cv.notify_one();
    }

    virtual cwipc_pointcloud* get() override final {
        std::lock_guard<std::mutex> lock(m_result_mutex);
        cwipc_pointcloud *rv = m_result;
        m_result = NULL;

        return rv;
    }

private:
    void alloc_decoder() {
        cwipc_encoder_params par;

        //Default codec parameter values set in signals
        par.do_inter_frame = false;
        par.gop_size = 1;
        par.exp_factor = 1.0;
        par.octree_bits = 8;
        par.jpeg_quality = 85;
        par.macroblock_size = 16;
        std::stringstream compfr;

        //Convert buffer to stringstream for encoding
        m_decoder_V2_ = pcl::shared_ptr<cwipc_pointcloud_codec> (
            new cwipc_pointcloud_codec (
                pcl::io::MANUAL_CONFIGURATION,
                false,
                std::pow ( 2.0, -1.0 * par.octree_bits ),
                std::pow ( 2.0, -1.0 * par.octree_bits),
                downdownsampling, // no intra voxel coding in this first version of the codec
                iframerate, // i_frame_rate,
                true,
                8,
                1,
                false,
                false, // not implemented
                false, // do_connectivity_coding_, not implemented
                par.jpeg_quality,
                num_threads
            )
        );
    }

    cwipc_pointcloud *m_result;
    std::mutex m_result_mutex;
    std::condition_variable m_result_cv;
    pcl::shared_ptr<cwipc_pointcloud_codec > m_decoder_V2_;
    bool m_alive;
};


//
// C-compatible entry points
//


const char *cwipc_get_version_codec() {
#ifdef CWIPC_VERSION
    return stringify(CWIPC_VERSION);
#else
    return "unknown";
#endif
}

cwipc_encoder* cwipc_new_encoder(int version, cwipc_encoder_params *params, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char *)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_new_encoder: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }

    if (version != CWIPC_ENCODER_PARAM_VERSION && version != CWIPC_ENCODER_PARAM_VERSION_OLD) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_new_encoder: incorrect encoder param version 0x%08x expected 0x%08x or 0x%08x", version, CWIPC_ENCODER_PARAM_VERSION, CWIPC_ENCODER_PARAM_VERSION_OLD);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    if (params == NULL) {
        static cwipc_encoder_params dft = {false, 1, 1.0, 9, 85, 16, 0, 0, 0};
        params = &dft;
    }

    // Check parameters for this release
    if (params->do_inter_frame) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_new_encoder", "inter-frame encoding not supported in this version");
        cwipc_log_set_errorbuf(NULL);
        return NULL;
    }

    if (params->gop_size != 1) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_new_encoder", "gop_size != 1 not supported in this version");
        cwipc_log_set_errorbuf(NULL);
        return NULL;
    }
    cwipc_encoder *rv = nullptr;
    if (version == CWIPC_ENCODER_PARAM_VERSION && params->n_parallel > 1) {
        rv = new cwipc_multithreaded_encoder_impl(params);
    } else {
        rv = new cwipc_encoder_impl(params);
    }
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_new_encoder", "failed without error message");
    }
    cwipc_log_set_errorbuf(NULL);
    return rv;
}

void cwipc_encoder_free(cwipc_encoder *obj) {
    obj->free();
}

bool cwipc_encoder_available(cwipc_encoder *obj, bool wait) {
    return obj->available(wait);
}

bool cwipc_encoder_eof(cwipc_encoder *obj) {
    return obj->eof();
}

void cwipc_encoder_feed(cwipc_encoder *obj, cwipc_pointcloud* pc) {
    obj->feed(pc);
}

void cwipc_encoder_close(cwipc_encoder *obj) {
    obj->close();
}

size_t cwipc_encoder_get_encoded_size(cwipc_encoder *obj) {
    return obj->get_encoded_size();
}

bool cwipc_encoder_copy_data(cwipc_encoder *obj, void *buffer, size_t bufferSize) {
    return obj->copy_data(buffer, bufferSize);
}

bool cwipc_encoder_at_gop_boundary(cwipc_encoder *obj) {
    return obj->at_gop_boundary();
}

cwipc_encodergroup *cwipc_new_encodergroup(char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_new_encodergroup: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_encodergroup *rv = new cwipc_encodergroup_impl();
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_new_encodergroup", "unspecified error");
    }
    cwipc_log_set_errorbuf(NULL);
    return rv;
}

void cwipc_encodergroup_free(cwipc_encodergroup *obj) {
    obj->free();
}

cwipc_encoder *cwipc_encodergroup_addencoder(cwipc_encodergroup *obj, int version, cwipc_encoder_params* params, char **errorMessage) {
    return obj->addencoder(version, params, errorMessage);
}

void cwipc_encodergroup_feed(cwipc_encodergroup *obj, cwipc_pointcloud* pc) {
    return obj->feed(pc);
}

void cwipc_encodergroup_close(cwipc_encodergroup *obj) {
    obj->close();
}

cwipc_decoder* cwipc_new_decoder(char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_new_decoder: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }

    cwipc_decoder *rv = new cwipc_decoder_impl();
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_new_decoder", "unspecified error");
    }
    cwipc_log_set_errorbuf(NULL);
    return rv;
}

void cwipc_decoder_feed(cwipc_decoder *obj, void *buffer, size_t bufferSize) {
    obj->feed(buffer, bufferSize);
}

void cwipc_decoder_close(cwipc_decoder *obj) {
    obj->close();
}
