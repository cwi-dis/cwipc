#include <iostream>
#include <fstream>

#include <pcl/point_cloud.h>
#include <pcl/io/ply_io.h>

#include "cwipc_util/api.h"

int main(int argc, char** argv) {
    char *message = NULL;
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << "pointcloudfile.ply pointcloudfile.cwipcdump" << std::endl;
        return 2;
    }

    //
    // Read pointcloud file
    //
    cwipc_pointcloud *pc = cwipc_read(argv[1], 0, &message, CWIPC_API_VERSION);
    if (pc == NULL) {
        std::cerr << argv[0] << ": Cannot read pointcloud: " << message << std::endl;
        return 1;
    }

    //
    // Save
    //
    int status = cwipc_write_debugdump(argv[2], pc, &message);
    if (status < 0) {
        std::cerr << argv[0] << ": Cannot save pointcloud to cwipcdump: " << message << std::endl;
        return 1;
    }
    pc->free();
    if (cwipc_dangling_allocations(true)) return 1;

    return 0;
}

