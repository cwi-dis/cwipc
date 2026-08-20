#include <iostream>
#include <fstream>
#include "string.h"
#include <stdlib.h>
#include <inttypes.h>
#include "cwipc_util/api.h"
#include "cwipc_orbbec/api.h"

int main(int argc, char** argv) {
    char *error = NULL;
    cwipc_source *generator = cwipc_orbbec("auto", &error, CWIPC_API_VERSION);
    bool want_hardware = argc > 1 && strcmp(argv[1], "--hardware") == 0;
    if (generator == NULL) {
        char* expectedError = strstr(error, "no cameras found");
        if (expectedError == NULL) {
            // Any other error is unexpected.
            std::cerr << argv[0] << ": Error: " << error << std::endl;
            return 1;
        }
        if (want_hardware) {
            return 1;
        }
    }
    else {
        generator->free();
    }
    return 0;
}

