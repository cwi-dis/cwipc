#include <iostream>
#include <fstream>
#include "string.h"
#include <stdlib.h>
#include <inttypes.h>

#include "cwipc_util/api.h"
#include "cwipc_kinect/api.h"

#undef DEBUG_METADATA
#undef DEBUG_CONFIG

int main(int argc, char** argv) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " count directory [configfile]" << std::endl;
        std::cerr << "Creates COUNT pointclouds from a kinect4a camera and stores the PLY files in the given DIRECTORY" << std::endl;
        std::cerr << "If directory is - then drop the pointclouds on the floor" << std::endl;

        return 2;
    }

    char *configFile = argv[3];
    int count = atoi(argv[1]);
    char filename[500];
    char *error = NULL;

    cwipc_activesource *generator;

    generator = cwipc_kinect_playback(configFile, &error, CWIPC_API_VERSION);

    if (generator == NULL) {
        std::cerr << argv[0] << ": creating realsense2_playback grabber failed: " << error << std::endl;

        return 1;
    }

#ifdef DEBUG_METADATA
    generator->request_metadata("rgb");
    generator->request_metadata("depth");
#endif

#ifdef DEBUG_CONFIG
    size_t configSize = generator->get_config(nullptr, 0);
    char* configBuf = (char*)malloc(configSize + 1);
    memset(configBuf, 0, configSize + 1);
    generator->get_config(configBuf, configSize);

    std::cerr << "cameraconfig as json:\n=================\n" << configBuf << "\n======================\n";
#endif

    cwipc_tileinfo tif;
    bool ok = generator->get_tileinfo(0, &tif);
    if (!ok) {
        std::cerr << argv[0] << ": get_tileinfo(0) failed" << std::endl;
        return 1;
    }
    generator->start();
    while (count-- > 0) {
        cwipc_pointcloud *pc = NULL;
        pc = generator->get();
        if (pc == NULL) {
            std::cerr << argv[0] << ": get() returned NULL. Try again." << std::endl;
            pc = generator->get();
            if (pc == NULL) {
                std::cerr << argv[0] << ": get() returned NULL on second attempt" << std::endl;
                ok = false;
                break;
            }
        }
        if (strcmp(argv[2], "-") != 0) {
            snprintf(filename, sizeof(filename), "%s/pointcloud-%" PRIu64 ".ply", argv[2], pc->timestamp());
            ok = cwipc_write(filename, pc, &error) == 0;
            if (!ok) {
                std::cerr << argv[0] << ": Error writing pointcloud to " << filename << ": " << error << std::endl;
                break;
            }
        }

#ifdef DEBUG_METADATA
        cwipc_metadata* ap = pc->access_metadata();

        if (ap == nullptr) {
            std::cerr << argv[0] << ": access_metadata: returned null pointer" << std::endl;
        } else {
            std::cerr << argv[0] << ": metadata: " << ap->count() << " items:" << std::endl;

            for (int i=0; i<ap->count(); i++) {
                const char *name = ap->name(i).c_str();
                const char *description = ap->description(i).c_str();
                size_t size = ap->size(i);
                void *pointer = ap->pointer(i);
                std::cerr << argv[0] << ": metadata: item " << i << " name=" << name << ", size=" << (int)size << ", description=" << description << ", pointer=0x" << std::hex << (uint64_t)pointer << std::endl;
            }
        }
#endif
        pc->free();
    }


    generator->free();

    if (!ok) {
        std::cerr << argv[0] << ": Error during test." << std::endl;
        return 1;
    }
    if (cwipc_dangling_allocations(true)) return 1;

    return 0;
}

