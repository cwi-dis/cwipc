#include <iostream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <vector>
#include <cstring>
#include <string.h>
#include "cwipc_codec/api.h"

//
// xxxjack note to self: Under XCode, the
// default working directory is cwipc/build-xcode/bin/Debug
// so ../../../cwipc_codec/tests/fixtures/input/loot_tiled.ply is
// a decent path for testing.
//
#define COUNT 50
#undef READ_MULTIPLE

char *progname;
char *filename;

cwipc_pointcloud* readpc(int index) {
    //
    // Read pointcloud file
    //
    char* errorMessage = NULL;
    char namebuf[1024];
    snprintf(namebuf, sizeof(namebuf), filename, index);
    cwipc_pointcloud *pc;

    if (strcmp(namebuf+strlen(namebuf)-4, ".ply") == 0) {
        pc = cwipc_read(namebuf, 0LL, &errorMessage, CWIPC_API_VERSION);
    } else {
        pc = cwipc_read_debugdump(namebuf, &errorMessage, CWIPC_API_VERSION);
    }

    if (pc == NULL || errorMessage) {
        std::cerr << "cwipc_enc_perftest: Error reading pointcloud from " << namebuf << ": " << errorMessage << std::endl;
        exit(1);
    }

    return pc;
}

int measure(std::vector<int>& all_octree_bits, std::vector<int>& all_jpeg_quality, std::vector<int> all_tilenumber) {
    cwipc_pointcloud* pc = nullptr;
    pc = readpc(0);

    std::cerr << progname << ": Read pointcloud, " << pc->count() << " points, " << pc->get_uncompressed_size() << " bytes (uncompressed)" << std::endl;

    cwipc_encoder_params param;
    memset(&param, 0, sizeof(param));
    param.do_inter_frame = false;
    param.gop_size = 1;
    param.exp_factor = 1.0;
    param.octree_bits = 0;
    param.jpeg_quality = 0;
    param.macroblock_size = 16;
    param.tilenumber = 0;
    param.voxelsize = 0;
    param.n_parallel = 0;

    char *errorString;
    cwipc_encodergroup *encodergroup = cwipc_new_encodergroup(&errorString, CWIPC_API_VERSION);

    if (encodergroup == NULL) {
        std::cerr << progname << ": Could not create encodergroup: " << errorString << std::endl;
        return 1;
    }

    std::vector<cwipc_encoder *> encoders;

    for(int octree_bits : all_octree_bits) {
        for(int jpeg_quality : all_jpeg_quality) {
            for(int tilenumber : all_tilenumber) {
                param.octree_bits = octree_bits;
                param.jpeg_quality = jpeg_quality;
                param.tilenumber = tilenumber;
                cwipc_encoder *e = encodergroup->addencoder(CWIPC_ENCODER_PARAM_VERSION, &param, &errorString);

                if (e == NULL) {
                    std::cerr << progname << ": Could not create encoder: " << errorString << std::endl;
                    return 1;
                }

                encoders.push_back(e);
            }
        }
    }

    auto t0_wall = std::chrono::high_resolution_clock::now();
    size_t totalBufSize = 0;
    int totalCount = 0;

    for (int i=0; i<COUNT; i++) {
        encodergroup->feed(pc);

#ifdef READ_MULTIPLE
        pc->free();
        pc = readpc(i + 1);
#endif

        for(cwipc_encoder *e: encoders) {
            bool ok = e->available(true);
            if (!ok) {
                std::cerr << progname << ": Encoder did not create compressed data" << std::endl;
                return 1;
            }

            size_t bufSize = e->get_encoded_size();
            totalBufSize += bufSize;
            totalCount++;
            char *buffer = (char *)malloc(bufSize);

            if (buffer == NULL) {
                std::cerr << progname << ": Could not allocate " << bufSize << " bytes." << std::endl;
                return 1;
            }

            ok = e->copy_data(buffer, bufSize);
            if (!ok) {
                std::cerr << progname << ": Encoder could not copy compressed data" << std::endl;
                return 1;
            }

            free(buffer);
        }
    }

    auto t1_wall = std::chrono::high_resolution_clock::now();

    double delta_wall = std::chrono::duration<double, std::milli>(t1_wall - t0_wall).count();
    delta_wall /= COUNT;
    std::cerr << progname << ": Compressed " << COUNT << " times using " << all_octree_bits.size()*all_jpeg_quality.size()*all_tilenumber.size() << " compressors , output: " << totalBufSize/COUNT << " bytes per input cloud, " << totalBufSize/totalCount << " average per output stream" << std::endl;
    std::cerr << std::fixed << std::setprecision(2) << progname << ": per iteration: " << delta_wall << "ms" << std::endl;

    pc->free();	// After feeding the pointcloud into the encoder we can free it.
    encodergroup->free(); // We don't need the encoder anymore.

    return 0;
}

int main(int argc, char** argv) {
    auto t0_wall = std::chrono::high_resolution_clock::now();
    uint64_t timestamp = 0LL;
    progname = argv[0];

    if (argc != 2) {
#ifdef READ_DEBUGDUMP
        std::cerr << "Usage: " << progname << " pointcloudfile.cwipcdump" << std::endl;
#else
        std::cerr << "Usage: " << progname << " pointcloudfile.ply" << std::endl;
#endif

        return 2;
    }
    filename = argv[1];

    //
    // Compress
    //
    {
        std::vector<int> all_octree_bits{ 9 };
        std::vector<int> all_jpeg_quality{ 85 };
        std::vector<int> all_tilenumber{ 0 };

        if (measure(all_octree_bits, all_jpeg_quality, all_tilenumber)) {
            return 1;
        }
    }

    {
        std::vector<int> all_octree_bits{ 9 };
        std::vector<int> all_jpeg_quality{ 85 };
        std::vector<int> all_tilenumber{ 1, 2, 3, 4 };

        if (measure(all_octree_bits, all_jpeg_quality, all_tilenumber)) {
            return 1;
        }
    }

    {
        std::vector<int> all_octree_bits{ 9, 6 };
        std::vector<int> all_jpeg_quality{ 85 };
        std::vector<int> all_tilenumber{ 1, 2, 3, 4 };

        if (measure(all_octree_bits, all_jpeg_quality, all_tilenumber)) {
            return 1;
        }
    }

    auto t1_wall = std::chrono::high_resolution_clock::now();
    double delta_wall = std::chrono::duration<double, std::milli>(t1_wall - t0_wall).count();

    std::cerr << std::fixed << std::setprecision(2) << progname << ": total runtime: " << delta_wall << "ms" << std::endl;
    if (cwipc_dangling_allocations(true)) return 1;

    return 0;
}
