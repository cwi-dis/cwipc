#include <iostream>
#include <fstream>

#include "cwipc_util/api.h"

int main(int argc, char** argv) {
    uint64_t timestamp = 0LL;
    if (argc != 6) {
        std::cerr << "Usage: " << argv[0] << " kNeighbors stddevMulThresh perTileBool pointcloudfile.ply newpointcloudfile.ply" << std::endl;
        std::cerr << "Example: " << argv[0] << " 20 1.0 1 pointcloudfile.ply newpointcloudfile.ply" << std::endl;
        return 2;
    }

    int kNeighbors = atoi(argv[1]);
    float stddevMulThresh = atof(argv[2]);
    bool perTile = atoi(argv[3]);

    //
    // Read pointcloud file
    //
    char *errorMessage = NULL;
    cwipc_pointcloud *pc = cwipc_read(argv[4], 0LL, &errorMessage, CWIPC_API_VERSION);

    if (pc == NULL || errorMessage) {
        std::cerr << argv[0] << ": Error reading pointcloud from " << argv[4] << ": " << errorMessage << std::endl;
        return 1;
    }
    std::cerr << "Read pointcloud successfully, " << pc->get_uncompressed_size() << " bytes (uncompressed)" << std::endl;

    //
    // Voxelize
    //
    cwipc_pointcloud *new_pc = cwipc_remove_outliers(pc, kNeighbors, stddevMulThresh, perTile);

    //
    // Save pointcloud file
    //
    if (cwipc_write(argv[5], new_pc, NULL) < 0) {
        std::cerr << argv[0] << ": Error writing PLY file " << argv[5] << std::endl;
        return 1;
    }

    pc->free();
    new_pc->free();
    if (cwipc_dangling_allocations(true)) return 1;

    return 0;
}

