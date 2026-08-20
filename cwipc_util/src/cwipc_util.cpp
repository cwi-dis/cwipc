#include <cstddef>
#include <stdio.h>
#include <inttypes.h>

#include <chrono>
#include <mutex>

#include <pcl/point_cloud.h>
#include <pcl/io/ply_io.h>
#include <pcl/common/geometry.h>


#ifdef WIN32
#define _CWIPC_UTIL_EXPORT __declspec(dllexport)
#else
#define _CWIPC_UTIL_EXPORT
#endif
#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/logging.hpp"
#define stringify(x) _stringify(x)
#define _stringify(x) #x

class cwipc_metadata_impl : public cwipc_metadata {
protected:
    struct item {
        std::string name;
        std::string description;
        void *pointer;
        size_t size;
        deallocfunc dealloc;
    };
    std::vector<struct item> m_items;

public:
    cwipc_metadata_impl() {}

    ~cwipc_metadata_impl() {
        for(auto item: m_items) {
            item.dealloc(item.pointer);
        }

        m_items.clear();
    }

    int count() override {
        return m_items.size();
    }

    const std::string& name(int idx) override {
        return m_items[idx].name;
    }

    const std::string& description(int idx) override {
        return m_items[idx].description;
    }

    void* pointer(int idx) override {
        return m_items[idx].pointer;
    }

    size_t size(int idx) override {
        return m_items[idx].size;
    }

    void _add(const std::string& name, const std::string& description, void *pointer, size_t size, deallocfunc dealloc) override {
        struct item new_item;

        new_item.name = name;
        new_item.description = description;
        new_item.pointer = pointer;
        new_item.size = size;
        new_item.dealloc = dealloc;

        m_items.push_back(new_item);
    }

    void _move(cwipc_metadata *other) override {
        auto other_impl = (cwipc_metadata_impl*)other;

        for(auto item: m_items) {
            other_impl->m_items.push_back(item);
        }

        m_items.clear();
    }
};

static std::mutex alloc_dealloc_mutex;
static int n_cwipc_pointcloud_alloc = 0;
static int n_cwipc_pointcloud_dealloc = 0;
static int n_cwipc_uncompressed_pointcloud_alloc = 0;
static int n_cwipc_uncompressed_pointcloud_dealloc = 0;
class cwipc_impl : public cwipc_pointcloud {
protected:
    uint64_t m_timestamp;
    float m_cellsize;
    cwipc_pcl_pointcloud m_pc;
    cwipc_metadata* m_metadata;

public:
    cwipc_impl() : m_timestamp(0), m_cellsize(0), m_pc(NULL), m_metadata(NULL) {}
    cwipc_impl(cwipc_pcl_pointcloud pc, uint64_t timestamp) : m_timestamp(timestamp), m_cellsize(0), m_pc(NULL), m_metadata(NULL)
    {
        if (pc) {
            std::lock_guard<std::mutex> lock(alloc_dealloc_mutex);
            m_pc = pc;
            n_cwipc_pointcloud_alloc++;
        }
    }

    ~cwipc_impl() {}

    cwipc_pointcloud *_shallowcopy() override {
        auto rv = new cwipc_impl(m_pc, m_timestamp);
        rv->_set_cellsize(m_cellsize);
        return rv;
    }

    int from_points(struct cwipc_point *pointData, size_t size, int npoint, uint64_t timestamp) {
        if (npoint * sizeof(struct cwipc_point) != size) {
            return -1;
        }

        m_timestamp = timestamp;
        cwipc_pcl_pointcloud pc = new_cwipc_pcl_pointcloud();
        pc->points.reserve(npoint);

        for (int i=0; i<npoint; i++) {
            cwipc_pcl_point point;

            point.x = pointData[i].x;
            point.y = pointData[i].y;
            point.z = pointData[i].z;
            point.r = pointData[i].r;
            point.g = pointData[i].g;
            point.b = pointData[i].b;
            point.a = pointData[i].tile;

            pc->points.push_back(point);
        }

        std::lock_guard<std::mutex> lock(alloc_dealloc_mutex);
        m_pc = pc;
        n_cwipc_pointcloud_alloc++;
        return npoint;
    }

    void free() override {
        {
            std::lock_guard<std::mutex> lock(alloc_dealloc_mutex);

            if (m_pc != NULL) {
                n_cwipc_pointcloud_dealloc++;
            }
            m_pc = NULL;
        }
        if (m_metadata) {
            delete m_metadata;
        }

        m_metadata = NULL;
    }

    uint64_t timestamp() override {
        return m_timestamp;
    }

    float cellsize() override {
        return m_cellsize;
    }

    void _set_cellsize(float cellsize) override {
        if (cellsize < 0 && m_pc) {
            // Guess cellsize by traversing over adjacent points
            // util we find 2 sets with minimum distance.
            float minDistance = std::numeric_limits<float>::infinity();
            auto prevPoint = m_pc->begin();

            for (auto it = m_pc->begin(); it != m_pc->end(); ++it) {
                if (it == prevPoint) {
                    continue;
                }

                float distance = pcl::geometry::distance(*it, *prevPoint);

                if (distance < minDistance) {
                    minDistance = distance;
#if 0
                } else if (distance == minDistance) {
                  break;
#endif
                } /* else continue */
            }

            if (minDistance == std::numeric_limits<float>::infinity()) {
                minDistance = 0;
            }

            cellsize = minDistance;
        }

        m_cellsize = cellsize;
    }

    void _set_timestamp(uint64_t timestamp) override {
        m_timestamp = timestamp;
    }

    int count() override {
        if (m_pc == nullptr) {
            cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_util", "count: NULL pointcloud");
            return 0;
        }
        return m_pc->size();
    }

    size_t get_uncompressed_size() override {
        if (m_pc == nullptr) {
            cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_util", "get_uncompressed_size: NULL pointcloud");
            return 0;
        }
        return m_pc->size() * sizeof(struct cwipc_point);
    }

    int copy_uncompressed(struct cwipc_point *pointData, size_t size) override {
        if (m_pc == nullptr) {
            cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_util", "copy_uncompressed: NULL pointcloud");
            return 0;
        }
        if (size < m_pc->size() * sizeof(struct cwipc_point)) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_util", "copy_uncompressed: buffer too small");
            return -1;
        }

        int npoint = m_pc->size();

        for (int i = 0; i < npoint; i++) {
            pointData[i].x = (*m_pc)[i].x;
            pointData[i].y = (*m_pc)[i].y;
            pointData[i].z = (*m_pc)[i].z;
            pointData[i].r = (*m_pc)[i].r;
            pointData[i].g = (*m_pc)[i].g;
            pointData[i].b = (*m_pc)[i].b;
            pointData[i].tile = (*m_pc)[i].a;
        }

        // iterate_over_octree();
        return npoint;
    }

    size_t copy_packet(uint8_t *packet, size_t size) override {
        if (m_pc == nullptr) {
            (void)access_pcl_pointcloud();
        }
        if (m_pc == nullptr) {
            cwipc_log(CWIPC_LOG_LEVEL_WARNING, "cwipc_util", "copy_packet: NULL pointcloud");
            return 0;
        }
        size_t dataSize = get_uncompressed_size();
        size_t sizeNeeded = sizeof(struct cwipc_cwipcdump_header) + dataSize;

        if (packet == NULL) {
            return sizeNeeded;
        }

        if (size != sizeNeeded) {
            return 0;
        }

        struct cwipc_cwipcdump_header hdr = {
            {
                CWIPC_CWIPCDUMP_HEADER[0],
                CWIPC_CWIPCDUMP_HEADER[1],
                CWIPC_CWIPCDUMP_HEADER[2],
                CWIPC_CWIPCDUMP_HEADER[3]
            },
            CWIPC_CWIPCDUMP_VERSION,
            timestamp(),
            cellsize(),
            0,
            dataSize
        };

        memcpy(packet, &hdr, sizeof(struct cwipc_cwipcdump_header));
        cwipc_point *pointData = (cwipc_point *)(packet + sizeof(struct cwipc_cwipcdump_header));
        copy_uncompressed(pointData, dataSize);

        return sizeNeeded;
    }

    cwipc_pcl_pointcloud access_pcl_pointcloud() override {
        return m_pc;
    }

    cwipc_metadata *access_metadata() override {
        if (m_metadata == NULL) {
            m_metadata = new cwipc_metadata_impl();
        }

        return m_metadata;
    }
};

//
// Another implementation of cwipc: by default stores the
// raw pointcloud data uncompressed, and only copies to a pcl octree
// if needed. Should be much more efficient in workflows where we only
// handle uncompressed data (no need to convert to/from octree representation,
// only do a memcpy).
//
class cwipc_uncompressed_impl : public cwipc_impl {
protected:
    struct cwipc_point* m_points;
    size_t m_points_size;

public:
    cwipc_uncompressed_impl() : cwipc_impl(), m_points(NULL), m_points_size(0) {}
    cwipc_uncompressed_impl(cwipc_pcl_pointcloud pc, uint64_t timestamp) : cwipc_impl(pc, timestamp), m_points(NULL), m_points_size(0) {}

    ~cwipc_uncompressed_impl() {}

    cwipc_pointcloud* _shallowcopy() override {
        // Ensure the m_pc is initialized
        (void)access_pcl_pointcloud();
        return cwipc_impl::_shallowcopy();
    }

    int from_points(struct cwipc_point *pointData, size_t size, int npoint, uint64_t timestamp) {
        if (npoint * sizeof(struct cwipc_point) != size) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_util", "from_points: size and npoint inconsistent");
            return -1;
        }

        m_timestamp = timestamp;
        m_points = (struct cwipc_point *)malloc(size);

        if (m_points == NULL) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_util", "from_points: could not allocate memory for points, size=" + std::to_string(size));
            return -1;
        }
        {
            std::lock_guard<std::mutex> lock(alloc_dealloc_mutex);
            n_cwipc_uncompressed_pointcloud_alloc++;
            m_points_size = size;
            if (m_pc != nullptr) {
                m_pc = nullptr;
                n_cwipc_pointcloud_dealloc++;
            }
        }
        memcpy(m_points, pointData, size);

        return npoint;
    }

    void free() override {
        cwipc_impl::free();

        if (m_points) {
            std::lock_guard<std::mutex> lock(alloc_dealloc_mutex);
            ::free(m_points);
            n_cwipc_uncompressed_pointcloud_dealloc++;
            m_points = NULL;
        }
        m_points_size = 0;
    }

    uint64_t timestamp() override {
        return m_timestamp;
    }

    float cellsize() override {
        return m_cellsize;
    }

    void _set_cellsize(float cellsize) override {
        (void)access_pcl_pointcloud();
        cwipc_impl::_set_cellsize(cellsize);
    }

    void _set_timestamp(uint64_t timestamp) override {
        m_timestamp = timestamp;
    }

    int count() override {
        return m_points_size / sizeof(struct cwipc_point);
    }

    size_t get_uncompressed_size() override {
        return m_points_size;
    }

    int copy_uncompressed(struct cwipc_point *pointData, size_t size) override {
        if (size != m_points_size) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_util", "copy_uncompressed: buffer too small");
            return -1;
        }

        memcpy(pointData, m_points, size);
        return m_points_size / sizeof(struct cwipc_point);
    }

    cwipc_pcl_pointcloud access_pcl_pointcloud() override {
        if (m_pc == nullptr) {
            cwipc_impl::from_points(m_points, m_points_size, m_points_size / sizeof(struct cwipc_point), m_timestamp);
        }

        return m_pc;
    }
};

const char *cwipc_get_version() {
#ifdef CWIPC_VERSION
    return stringify(CWIPC_VERSION);
#else
    return "unknown";
#endif
}

int cwipc_dangling_allocations(bool log) {
    int n_dangling = n_cwipc_pointcloud_alloc - n_cwipc_pointcloud_dealloc;
    int n_dangling_uncompressed = n_cwipc_uncompressed_pointcloud_alloc - n_cwipc_uncompressed_pointcloud_dealloc;
    if (log && n_dangling != 0 || n_dangling_uncompressed != 0) {
        std::string msg = std::to_string(n_dangling) + " free() mismatch. nAlloc=" + std::to_string(n_cwipc_pointcloud_alloc) + ", nFree=" + std::to_string(n_cwipc_pointcloud_dealloc);
        _cwipc_log_emit(CWIPC_LOG_LEVEL_WARNING, "cwipc_pointcloud", msg.c_str());
        msg = std::to_string(n_dangling_uncompressed) + " free() mismatch. nAlloc=" + std::to_string(n_cwipc_uncompressed_pointcloud_alloc) + ", nFree=" + std::to_string(n_cwipc_uncompressed_pointcloud_dealloc);
        _cwipc_log_emit(CWIPC_LOG_LEVEL_WARNING, "cwipc_pointcloud_uncompressed", msg.c_str());
    }
    return std::abs(n_dangling)+std::abs(n_dangling_uncompressed);
}

cwipc_pointcloud *cwipc_read(const char *filename, uint64_t timestamp, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char *msgbuf = (char *)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_read: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_pcl_pointcloud pc = new_cwipc_pcl_pointcloud();
    pcl::PLYReader ply_reader;

    if (ply_reader.read(filename, *pc) < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read", std::string("Loading of PLY file failed: ") + filename);
        cwipc_log_set_errorbuf(nullptr);

        return NULL;
    }

    cwipc_pointcloud *rv = new cwipc_impl(pc, timestamp);
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read", "unspecified error creating point cloud");
    }
    cwipc_log_set_errorbuf(nullptr);
    return rv;
}

int cwipc_write(const char *filename, cwipc_pointcloud *pointcloud, char **errorMessage) {
    cwipc_pcl_pointcloud pc = pointcloud->access_pcl_pointcloud();
    cwipc_log_set_errorbuf(errorMessage);
    if (pc == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write", "Saving NULL pointcloud not implemented");
        cwipc_log_set_errorbuf(nullptr);
        return -1;
    }

    pcl::PLYWriter writer;
    int status = writer.write(filename, *pc);

    if (status < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write", std::string("Saving of PLY file failed: ") + filename);
    }
    cwipc_log_set_errorbuf(nullptr);
    return status;
}

int cwipc_write_ext(const char* filename, cwipc_pointcloud* pointcloud, int flag, char** errorMessage) {
    cwipc_pcl_pointcloud pc = pointcloud->access_pcl_pointcloud();
    cwipc_log_set_errorbuf(errorMessage);
    if (pc == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write_ext", "Saving NULL pointcloud not implemented");
        cwipc_log_set_errorbuf(nullptr);
        return -1;
    }

    pcl::PLYWriter writer;
    int status = writer.write(filename, *pc, flag&CWIPC_FLAG_BINARY);

    if (status < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write_ext", std::string("Saving of PLY file failed: ") + filename);
    }
    cwipc_log_set_errorbuf(nullptr);
    return status;
}

cwipc_pointcloud* cwipc_read_debugdump(const char *filename, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_read_debugdump: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    FILE *fp = fopen(filename, "rb");

    if (fp == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read_debugdump", std::string("Cannot open file: ") + filename);
        cwipc_log_set_errorbuf(nullptr);

        return NULL;
    }

    struct cwipc_cwipcdump_header hdr;

    if (fread(&hdr, 1, sizeof(hdr), fp) != sizeof(hdr)) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read_debugdump", std::string("Cannot read pointcloud dumpfile header: ") + filename);
        cwipc_log_set_errorbuf(nullptr);
        fclose(fp);
        return NULL;
    }

    if (hdr.hdr[0] != CWIPC_CWIPCDUMP_HEADER[0] || hdr.hdr[1] != CWIPC_CWIPCDUMP_HEADER[1] || hdr.hdr[2] != CWIPC_CWIPCDUMP_HEADER[2] || hdr.hdr[3] != CWIPC_CWIPCDUMP_HEADER[3]) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read_debugdump", std::string("Pointcloud dumpfile header incorrect: ") + filename);
        cwipc_log_set_errorbuf(nullptr);

        fclose(fp);
        return NULL;
    }

    if (hdr.magic != CWIPC_CWIPCDUMP_VERSION) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read_debugdump", std::string("Pointcloud dumpfile version incorrect: ") + filename);
        cwipc_log_set_errorbuf(nullptr);

        fclose(fp);
        return NULL;
    }

    uint64_t timestamp = hdr.timestamp;
    size_t dataSize = hdr.size;
    float cellsize = hdr.cellsize;
    int npoint = dataSize / sizeof(cwipc_point);

    if (npoint*sizeof(cwipc_point) != dataSize) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read_debugdump", "Pointcloud dumpfile datasize inconsistent");
        cwipc_log_set_errorbuf(nullptr);
        fclose(fp);
        return NULL;
    }

    cwipc_point* pointData = (cwipc_point *)malloc(dataSize);

    if (pointData == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read_debugdump", "Could not allocate memory for point data");
        cwipc_log_set_errorbuf(nullptr);
        fclose(fp);
        return NULL;
    }

    if (fread(pointData, 1, dataSize, fp) != dataSize) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_read_debugdump", "Could not read point data of correct size");
        cwipc_log_set_errorbuf(nullptr);
        fclose(fp);
        return NULL;
    }

    fclose(fp);
    cwipc_uncompressed_impl *pc = new cwipc_uncompressed_impl();

    pc->from_points(pointData, dataSize, npoint, timestamp);
    pc->_set_cellsize(cellsize);
    free(pointData);

    return pc;
}

int cwipc_write_debugdump(const char *filename, cwipc_pointcloud *pointcloud, char **errorMessage) {
    size_t dataSize = pointcloud->get_uncompressed_size();
    struct cwipc_point *dataBuf = (struct cwipc_point *)malloc(dataSize);
    cwipc_log_set_errorbuf(errorMessage);
    if (dataBuf == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write_debugdump", "Cannot allocate memory, size="+ std::to_string(dataSize));
        cwipc_log_set_errorbuf(nullptr);
        return -1;
    }

    int nPoint = pointcloud->copy_uncompressed(dataBuf, dataSize);
    if (nPoint < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write_debugdump", "Cannot copy points, size="+ std::to_string(dataSize));
        cwipc_log_set_errorbuf(nullptr);
        free(dataBuf);

        return -1;
    }

    FILE *fp = fopen(filename, "wb");

    if (fp == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write_debugdump", std::string("Cannot open output file: ") + filename);
        cwipc_log_set_errorbuf(nullptr);
        free(dataBuf);

        return -1;
    }

    struct cwipc_cwipcdump_header hdr = {
        {
            CWIPC_CWIPCDUMP_HEADER[0],
            CWIPC_CWIPCDUMP_HEADER[1],
            CWIPC_CWIPCDUMP_HEADER[2],
            CWIPC_CWIPCDUMP_HEADER[3]
        },
        CWIPC_CWIPCDUMP_VERSION,
        pointcloud->timestamp(),
        pointcloud->cellsize(),
        0,
        dataSize
    };

    fwrite(&hdr, sizeof(hdr), 1, fp);

    if (fwrite(dataBuf, sizeof(struct cwipc_point), nPoint, fp) != nPoint) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_write_debugdump", "Cannot write point data, nPoint="+ std::to_string(nPoint));
        cwipc_log_set_errorbuf(nullptr);

        fclose(fp);
        free(dataBuf);

        return -1;
    }

    fclose(fp);
    free(dataBuf);
    cwipc_log_set_errorbuf(nullptr);
    return 0;
}

cwipc_pointcloud* cwipc_from_pcl(cwipc_pcl_pointcloud pc, uint64_t timestamp, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_from_pcl: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_pointcloud *rv = new cwipc_impl(pc, timestamp);
    if (rv == nullptr && errorMessage && *errorMessage == NULL) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_from_pcl", "unspecified error creating point cloud");
    }
    cwipc_log_set_errorbuf(nullptr);
    return rv;
}

cwipc_pointcloud* cwipc_from_points(cwipc_point* points, size_t size, int npoint, uint64_t timestamp, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_from_points: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }
        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    cwipc_uncompressed_impl *rv = new cwipc_uncompressed_impl();

    if (rv->from_points(points, size, npoint, timestamp) < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_from_points", "cannot load points (size error?)");
        cwipc_log_set_errorbuf(nullptr);

        delete rv;
        return NULL;
    }

    return rv;
}

cwipc_pointcloud* cwipc_from_packet(uint8_t *packet, size_t size, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_from_packet: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    struct cwipc_cwipcdump_header *header = (struct cwipc_cwipcdump_header *) packet;
    struct cwipc_point *points = (struct cwipc_point *)(packet + sizeof(cwipc_cwipcdump_header));

    if (memcmp(header->hdr, CWIPC_CWIPCDUMP_HEADER, 4) != 0 || header->magic != CWIPC_CWIPCDUMP_VERSION) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_util", "cwipc_from_packet: incorrect packet header or version");
        cwipc_log_set_errorbuf(nullptr);

        return NULL;
    }

    size_t dataSize = size - sizeof(struct cwipc_cwipcdump_header);
    int npoint = header->size / sizeof(cwipc_point);

    if (npoint * sizeof(cwipc_point) != dataSize) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_util", "cwipc_from_packet: inconsistent dataSize");
        cwipc_log_set_errorbuf(nullptr);

        return NULL;
    }

    cwipc_uncompressed_impl *rv = new cwipc_uncompressed_impl();

    if (rv->from_points(points, dataSize, npoint, header->timestamp) < 0) {
        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_from_packet", "cannot load points (size error?)");
        cwipc_log_set_errorbuf(nullptr);

        delete rv;
        return NULL;
    }

    rv->_set_cellsize(header->cellsize);
    cwipc_log_set_errorbuf(nullptr);
    return rv;
}

void cwipc_pointcloud_free(cwipc_pointcloud *pc) {
    pc->free();
}

cwipc_pointcloud *cwipc_pointcloud__shallowcopy(cwipc_pointcloud* pc) {
    return pc->_shallowcopy();
}

uint64_t cwipc_pointcloud_timestamp(cwipc_pointcloud *pc) {
    return pc->timestamp();
}

float cwipc_pointcloud_cellsize(cwipc_pointcloud *pc) {
    return pc->cellsize();
}

void cwipc_pointcloud__set_cellsize(cwipc_pointcloud *pc, float cellsize) {
    pc->_set_cellsize(cellsize);
}

void cwipc_pointcloud__set_timestamp(cwipc_pointcloud *pc, uint64_t timestamp) {
    pc->_set_timestamp(timestamp);
}

int cwipc_pointcloud_count(cwipc_pointcloud *pc) {
    return pc->count();
}

size_t cwipc_pointcloud_get_uncompressed_size(cwipc_pointcloud *pc) {
    return pc->get_uncompressed_size();
}

int cwipc_pointcloud_copy_uncompressed(cwipc_pointcloud *pc, struct cwipc_point *points, size_t size) {
    return pc->copy_uncompressed(points, size);
}

size_t cwipc_pointcloud_copy_packet(cwipc_pointcloud *pc, uint8_t *packet, size_t size) {
    return pc->copy_packet(packet, size);
}

cwipc_metadata* cwipc_pointcloud_access_metadata(cwipc_pointcloud *pc) {
    return pc->access_metadata();
}

void cwipc_metadata__move(cwipc_metadata *src, cwipc_metadata* dest) {
    src->_move(dest);
}

int cwipc_metadata_count(cwipc_metadata *collection) {
    return collection->count();
}

const char* cwipc_metadata_name(cwipc_metadata *collection, int idx) {
    return collection->name(idx).c_str();
}

const char* cwipc_metadata_description(cwipc_metadata *collection, int idx) {
    return collection->description(idx).c_str();
}

void * cwipc_metadata_pointer(cwipc_metadata *collection, int idx) {
    return collection->pointer(idx);
}

size_t cwipc_metadata_size(cwipc_metadata *collection, int idx) {
    return collection->size(idx);
}

bool cwipc_activesource_start(cwipc_activesource *src) {
    return src->start();
}

void cwipc_activesource_stop(cwipc_activesource *src) {
    src->stop();
}

cwipc_pointcloud* cwipc_source_get(cwipc_source *src) {
    return src->get();
}

void cwipc_source_free(cwipc_source *src) {
    src->free();
}

bool cwipc_source_eof(cwipc_source *src) {
  return src->eof();
}

bool cwipc_source_available(cwipc_source *src, bool wait) {
  return src->available(wait);
}

void cwipc_activesource_request_metadata(cwipc_activesource *src, const char *name) {
    src->request_metadata(name);
}

bool cwipc_activesource_is_metadata_requested(cwipc_activesource *src, const char *name) {
    return src->is_metadata_requested(name);
}

bool cwipc_activesource_reload_config(cwipc_activesource* src, const char* configFile) {
    return src->reload_config(configFile);
}

bool cwipc_activesource_reload_config_fast(cwipc_activesource* src, const char* configFile) {
    return src->reload_config_fast(configFile);
}

size_t cwipc_activesource_get_config(cwipc_activesource* src, char* buffer, size_t size) {
    return src->get_config(buffer, size);
}

bool cwipc_activesource_seek(cwipc_activesource* src, uint64_t timestamp)
{
    return src->seek(timestamp);
}

int cwipc_activesource_maxtile(cwipc_activesource *src) {
  return src->maxtile();
}

bool cwipc_activesource_get_tileinfo(cwipc_activesource *src, int tilenum, struct cwipc_tileinfo *tileinfo) {
  return src->get_tileinfo(tilenum, tileinfo);
}

bool cwipc_activesource_auxiliary_operation(cwipc_activesource *src, const char* op, const void* inbuf, size_t insize, void* outbuf, size_t outsize) {
    return src->auxiliary_operation(std::string(op), inbuf, insize, outbuf, outsize);
}

void cwipc_sink_free(cwipc_sink *sink) {
    sink->free();
}

bool cwipc_sink_feed(cwipc_sink *sink, cwipc_pointcloud *pc, bool clear) {
    return sink->feed(pc, clear);
}

bool cwipc_sink_caption(cwipc_sink *sink, const char *caption) {
    return sink->caption(caption);
}

char cwipc_sink_interact(cwipc_sink *sink, const char *prompt, const char *responses, int32_t millis) {
    return sink->interact(prompt, responses, millis);
}
