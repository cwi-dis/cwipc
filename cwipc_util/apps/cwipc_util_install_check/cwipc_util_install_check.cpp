#include <iostream>
#include <fstream>
#include "string.h"
#include <stdlib.h>
#include <inttypes.h>
#include "cwipc_util/api.h"

int main(int argc, char** argv) {
    char *error = NULL;
    cwipc_source *generator = cwipc_synthetic(0, 0, &error, CWIPC_API_VERSION);
    if (generator == NULL) {
        std::cerr << argv[0] << ": Error: " << error << std::endl;
        return 1;
    }
    return 0;
}

