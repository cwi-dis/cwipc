#include <iostream>
#include <fstream>

#include "cwipc_util/api.h"

int main(int argc, char** argv) {
    int fps = 0;
    int npoints = 0;

    if (argc >= 2) {
        fps = atoi(argv[1]);
    }

    if (argc >= 3) {
        npoints = atoi(argv[2]);
    }

    if (argc >= 4 || fps < 0 || npoints < 0) {
        std::cerr << "Usage: " << argv[0]  << " [fps [npoints]]" <<  std::endl;
        std::cerr << "Create synthetic pointclouds and show them in a window" << std::endl;
        return 2;
    }

    char *error;

    cwipc_source *generator = cwipc_synthetic(fps, npoints, &error, CWIPC_API_VERSION);
    if (generator == NULL) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    }

    cwipc_sink *window = cwipc_window("cwipc_viewsynthetic", &error, CWIPC_API_VERSION);
    if (window == NULL) {
        std::cerr << "Error: " << error << std::endl;
        return 1;
    }

    while (true) {
        cwipc_pointcloud *pc = generator->get();

        if (pc == NULL) {
            std::cerr << "Error: generator->get() returned NULL" << std::endl;
            return 1;
        }

        bool ok = window->feed(pc, true);
        if (!ok) {
            std::cerr << "Error: window->feed() returned false" << std::endl;
            return 1;
        }

        pc->free();
        char response = window->interact("Type q to quit", "q", 30);

        if (response == 'q') {
            break;
        }
    }

    window->free();
    generator->free();
    if (cwipc_dangling_allocations(true)) return 1;

    return 0;
}

