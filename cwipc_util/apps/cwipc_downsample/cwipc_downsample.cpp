#include <iostream>
#include <fstream>

#include "cwipc_util/api.h"

int main(int argc, char** argv) {
    uint64_t timestamp = 0LL;

    if (argc != 4) {
        std::cerr << "Usage: " << argv[0] << " voxelsize pointcloudfile.ply newpointcloudfile.ply" << std::endl;
        return 2;
    }

    float voxelsize = atof(argv[1]);

    //
    // Read pointcloud file
    //
    char *errorMessage = NULL;
    cwipc_pointcloud *pc = cwipc_read(argv[2], 0LL, &errorMessage, CWIPC_API_VERSION);

    if (pc == NULL || errorMessage) {
        std::cerr << argv[0] << ": Error reading pointcloud from " << argv[2] << ": " << errorMessage << std::endl;
        return 1;
    }

    std::cerr << "Read pointcloud successfully, " << pc->get_uncompressed_size() << " bytes (uncompressed)" << std::endl;
    //
    // Voxelize
    //
    cwipc_pointcloud *new_pc = cwipc_downsample(pc, voxelsize);

    //
    // Save pointcloud file
    //
    if (cwipc_write(argv[3], new_pc, NULL) < 0) {
      std::cerr << argv[0] << ": Error writing PLY file " << argv[3] << std::endl;
      return 1;
    }

    pc->free();
    new_pc->free();
    if (cwipc_dangling_allocations(true)) return 1;
    return 0;
}

