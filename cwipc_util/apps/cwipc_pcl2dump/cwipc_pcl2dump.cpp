#include <iostream>
#include <fstream>

#include <pcl/point_cloud.h>
#include <pcl/io/ply_io.h>

#include "cwipc_util/api_pcl.h"
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
    cwipc_pcl_pointcloud pcl_pc = new_cwipc_pcl_pointcloud();
    pcl::PLYReader ply_reader;

    if (ply_reader.read(argv[1], *pcl_pc) < 0) {
        std::cerr << argv[0] << ": Error reading pointcloud from " << argv[1] << std::endl;
        return 1;
    }
    std::cerr << "Read pointcloud successfully, " << pcl_pc->size() << " points." << std::endl;

    //
    // Turn into cwipc object
    //
    cwipc_pointcloud *pc = cwipc_from_pcl(pcl_pc, 0, &message, CWIPC_API_VERSION);
    if (pc == NULL) {
        std::cerr << argv[0] << ": Cannot convert pointcloud to cwipc: " << message << std::endl;
        return 1;
    }

    //
    // Save
    //
    if (strcmp(argv[2], "-") == 0) {
        // copy-uncompressed in stead of save (for performance testing)
        size_t nbytes = pc->get_uncompressed_size();
        cwipc_point *points = (cwipc_point *)malloc(nbytes);

        if (points == NULL) {
            std::cerr << argv[0] << ": out of memory" << std::endl;
            return 1;
        }

        pc->copy_uncompressed(points, nbytes);
        std::cerr << argv[0] << ": Skipping save" << std::endl;
    } else {
        int status = cwipc_write_debugdump(argv[2], pc, &message);

        if (status < 0) {
            std::cerr << argv[0] << ": Cannot save pointcloud to cwipcdump: " << message << std::endl;
            return 1;
        }
    }
    pc->free();
    if (cwipc_dangling_allocations(true)) return 1;

    return 0;
}

