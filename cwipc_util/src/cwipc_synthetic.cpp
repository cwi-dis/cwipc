#include <chrono>
#include <thread>
#include <inttypes.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/common/transforms.h>
#include <pcl/common/common_headers.h>

#ifdef WIN32
#define _CWIPC_UTIL_EXPORT __declspec(dllexport)
#else
#define _CWIPC_UTIL_EXPORT
#endif

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/logging.hpp"

class cwipc_source_synthetic_impl : public cwipc_activesource {
private:
    float m_angle;
    std::chrono::system_clock::time_point m_start;
    std::chrono::system_clock::time_point m_earliest_next_pointcloud;
    int m_hsteps;
    int m_asteps;
    int m_fps;
    cwipc_point *m_points;
    size_t m_points_size;
    bool m_started;

public:
    cwipc_source_synthetic_impl(int fps=0, int npoints=0)
    : m_angle(0),
      m_started(false),
      m_hsteps(0),
      m_asteps(0),
      m_fps(fps),
      m_points(NULL),
      m_points_size(0)
    {
        if (npoints == 0) {
            npoints = 160000;
        }

        m_hsteps = m_asteps = int(sqrt(npoints));
        npoints = m_hsteps * m_asteps;
        m_points_size = npoints*sizeof(cwipc_point);
        m_points = (cwipc_point *)malloc(m_points_size);
    }

    ~cwipc_source_synthetic_impl() {
        free();
    }

    virtual void free() override final {
        if (m_points) {
            ::free(m_points);
        }

        m_points = NULL;
    }

    virtual bool reload_config(const char* configFile) override final {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_synthetic", "reload_config() not implemented (nor needed)");
        return false;
    }

    virtual bool reload_config_fast(const char* configFile) override final {
        cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_synthetic", "reload_config_fast() not implemented (nor needed)");
        return false;
    }

    virtual size_t get_config(char* buffer, size_t size) override final {
        return 0;
    }
    
    virtual bool start() override final {
        if (m_started) {
            cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_synthetic", "start() called when already started");
            return true;
        }
        m_start = std::chrono::system_clock::now();
        m_earliest_next_pointcloud = m_start;
        m_started = true;
        return true;
    }

    virtual void stop() override final {
        m_started = false;
    }

    virtual bool eof() override final {
        return false;
    }

    virtual bool seek(uint64_t timestamp) override final {
        return false;
    }   

    virtual bool available(bool wait) override final {
        if (!m_started) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_synthetic", "available() called before start()");
            return false;
        }
        if (
            !wait && m_fps != 0 && 
            m_earliest_next_pointcloud.time_since_epoch() != std::chrono::milliseconds(0) &&
            std::chrono::system_clock::now() < m_earliest_next_pointcloud
        ) {
            return false;
        }
        return true;
    }

    virtual cwipc_pointcloud* get() override final {
        if (!m_started) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_synthetic", "get() called before start()");
            return NULL;
        }
        if (m_fps != 0 && m_earliest_next_pointcloud.time_since_epoch() != std::chrono::milliseconds(0)) {
            std::this_thread::sleep_until(m_earliest_next_pointcloud);
        }

        uint64_t timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
        std::chrono::duration<float, std::ratio<1>> runtime = std::chrono::system_clock::now() - m_start;

        if (m_fps != 0) {
            m_earliest_next_pointcloud = std::chrono::system_clock::now() + std::chrono::milliseconds(1000 / m_fps);
        }

        m_angle = runtime.count();
        generate_points();
        cwipc_pointcloud *rv = cwipc_from_points(m_points, m_points_size, m_hsteps*m_asteps, timestamp, NULL, CWIPC_API_VERSION);

        if (rv) {
            rv->_set_cellsize(2.0 / m_hsteps);

            // For testing purposes: save angle if wanted
            if (is_metadata_requested("test-angle")) {
                void *memptr = malloc(sizeof(m_angle));
                memcpy(memptr, &m_angle, sizeof(m_angle));
                cwipc_metadata *ap = rv->access_metadata();
                ap->_add("test-angle", "", memptr, sizeof(m_angle), ::free);
            }
        }

        return rv;
    }

    virtual int maxtile() override final {
        return 3;
    }

    virtual bool get_tileinfo(int tilenum, struct cwipc_tileinfo *tileinfo) override final{
        static cwipc_tileinfo syntheticInfo[3] = {
            {{0, 0, 0}, (char *)"synthetic", 2, 0},
            {{0, 0, 1}, (char *)"synthetic-right", 1, 1},
            {{0, 0, -1}, (char *)"synthetic-left", 1, 2},
        };

        switch(tilenum) {
        case 0:
        case 1:
        case 2:
            if (tileinfo) {
                *tileinfo = syntheticInfo[tilenum];
            }
            return true;
        }

        return false;
    }

    virtual bool auxiliary_operation(const std::string op, const void* inbuf, size_t insize, void* outbuf, size_t outsize) override final{
        // For test purposes, really...
        if (op != "test-setangle") return false;
        if (inbuf == nullptr || insize != sizeof(float)) return false;
        if (outbuf == nullptr || outsize != sizeof(float)) return false;
        float *infloat = (float *)inbuf;
        float *outfloat = (float *)outbuf;
        m_angle = *infloat;
        *outfloat = m_angle;
        return true;
    }

private:
    void generate_points() {
        const float pi=3.14159265358979f;
        const float max_height = 2.0;
        const float delta_h = max_height / m_hsteps;
        const float delta_a = 2*pi / m_asteps;

        cwipc_point *pptr = m_points;

        for (int height_i=0; height_i < m_hsteps; height_i++) {
            float height = height_i * delta_h;

            for (int angle_i=0; angle_i < m_asteps; angle_i++) {
                float angle = angle_i * delta_a;
                float radius = 0.3* pow(cos(height*pi/3-pi/6), 0.71);
                float x = radius*sin(angle);
                float y = radius*cos(angle);
                float r = (1+sin(2*pi*height+m_angle+angle))/2;
                float g = (1+sin(3*pi*height+m_angle+angle))/2;
                float b = (1+sin(4*pi*height+m_angle+angle))/2;
                int rr = (int)(r*255.0);
                int gg = (int)(g*255.0);
                int bb = (int)(b*255.0);

                // Eyes
                if (height > 1.7 && height < 1.8 && ((angle > pi*0.083 && angle < pi*0.1667) || (angle > pi*1.833 && angle < pi*1.917))) {
                    if (fmod(m_angle, pi/2) > 0.08) {
                        rr = gg = bb = 255;
                    }
                }

                pptr->x = -x;
                pptr->y = height;
                pptr->z = y;
                pptr->r = rr;
                pptr->g = gg;
                pptr->b = bb;
                pptr->tile = y < 0 ? 1 : 2;
                pptr++;
            }
        }
    }
};

cwipc_activesource* cwipc_synthetic(int fps, int npoints, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_synthetic: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_activesource *rv = new cwipc_source_synthetic_impl(fps, npoints);
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_synthetic", "unspecified error");
    }
    cwipc_log_set_errorbuf(nullptr);
    return rv;
}
