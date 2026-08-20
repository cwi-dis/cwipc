#include <iostream>
#include <fstream>
#include "string.h"
#include <stdlib.h>
#include <inttypes.h>

#include "cwipc_util/api.h"

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " count directory" << std::endl;
        std::cerr << "Creates COUNT synthetic pointclouds and stores the PLY files in the given DIRECTORY" << std::endl;
        return 2;
    }

    int count = atoi(argv[1]);
    char filename[500];
    char *error;

    cwipc_activesource *generator = cwipc_synthetic(0, 0, &error, CWIPC_API_VERSION);
    if (generator == NULL) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    }
    generator->start();
    int ok = 0;
    while (count-- > 0 && ok == 0) {
        cwipc_pointcloud *pc = generator->get();
        if (pc == NULL) {
            std::cerr << "Error: generator returned NULL pointcloud" << std::endl;
            return 1;
        }
        snprintf(filename, sizeof(filename), "%s/pointcloud-%" PRIu64 ".ply", argv[2], pc->timestamp());
        ok = cwipc_write(filename, pc, &error);
        pc->free();
    }

    if (ok < 0) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    }
    if (cwipc_dangling_allocations(true)) return 1;

    return 0;
}

