#include <chrono>
#include <thread>
#include <fstream>
#include <inttypes.h>
#include <pcl/point_types.h>
#include <pcl/point_cloud.h>
#include <pcl/common/transforms.h>
#include <pcl/common/common_headers.h>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

#ifdef WIN32
#define _CWIPC_UTIL_EXPORT __declspec(dllexport)
#else
#define _CWIPC_UTIL_EXPORT
#endif

#include "cwipc_util/api_pcl.h"
#include "cwipc_util/api.h"
#include "cwipc_util/internal/capturers.hpp"

struct capturer {
    std::string name;
    _cwipc_functype_count_devices* countFunc;
    _cwipc_func_capturer_factory* factoryFunc;
};

std::vector<struct capturer> all_capturers;

cwipc_activesource *cwipc_capturer(const char *configFilename, char **errorMessage, uint64_t apiVersion) {
    if (apiVersion < CWIPC_API_VERSION_OLD || apiVersion > CWIPC_API_VERSION) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_capturer: incorrect apiVersion 0x%08" PRIx64 " expected 0x%08" PRIx64 "..0x%08" PRIx64 "", apiVersion, CWIPC_API_VERSION_OLD, CWIPC_API_VERSION);
            *errorMessage = msgbuf;
        }

        return NULL;
    }
    cwipc_log_set_errorbuf(errorMessage);
    
    if (configFilename == nullptr || *configFilename == '\0') {
        configFilename = "cameraconfig.json";
    }

    if (strcmp(configFilename, "auto") == 0) {
        // Find any capturer that corresponds to a physical device (i.e. has a count function) that is available
        struct capturer* candidate = nullptr;

        for (auto& c : all_capturers) {
            if (c.countFunc != nullptr) {
                if (c.countFunc()) {
                    // This is a candidate. Check that there are no others.
                    if (candidate == nullptr) {
                        candidate = &c;
                    } else {
                        cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_capturer", "auto: multiple supported cameras found");
                        cwipc_log_set_errorbuf(nullptr);
                        return nullptr;
                    }
                }
            }
        }

        if (candidate == nullptr) {
            cwipc_log(CWIPC_LOG_LEVEL_ERROR, "cwipc_capturer", "auto: no supported cameras found");
            cwipc_log_set_errorbuf(nullptr);

            return nullptr;
        }

        return candidate->factoryFunc(configFilename, errorMessage, apiVersion);
    }

    // Assure we are passed a json cameraconfig
    std::string cameraType;
    json json_data;

    if (configFilename[0] == '{') {
        // Special case: a string starting with { is considered a JSON literal

        try {
            json_data = json::parse(configFilename);
        } catch (const std::exception& e) {
            if (errorMessage) {
                char* msgbuf = (char*)malloc(1024);
                snprintf(msgbuf, 1024, "cwipc_capturer: auto: JSON parse error");
                *errorMessage = msgbuf;
            }

            return nullptr;
        }
    } else {
        // Otherwise it is a filename. we check the extension. we only support .json.
        const char* extension = strrchr(configFilename, '.');
        if (strcmp(extension, ".json") == 0) {
            try {
                std::ifstream f(configFilename);

                if (!f.is_open()) {
                    if (errorMessage) {
                        char* msgbuf = (char*)malloc(1024);
                        snprintf(msgbuf, 1024, "cwipc_capturer: auto: cannot open \"%s\"", configFilename);
                        *errorMessage = msgbuf;
                    }

                    return nullptr;
                }

                json_data = json::parse(f);
            } catch (const std::exception& e) {
                if (errorMessage) {
                    char* msgbuf = (char*)malloc(1024);
                    snprintf(msgbuf, 1024, "cwipc_capturer: auto: JSON parse error");
                    *errorMessage = msgbuf;
                }

                return nullptr;
            }
        }
    }

    if (!json_data.contains("type")) {
        if (errorMessage) {
            char* msgbuf = (char*)malloc(1024);
            snprintf(msgbuf, 1024, "cwipc_capturer: auto: cannot determine camera type from \"%s\"", configFilename);
            *errorMessage = msgbuf;
        }

        return nullptr;
    }

    std::string type;
    json_data.at("type").get_to(type);

    for (auto& c: all_capturers) {
        if (c.name == type) {
            return c.factoryFunc(configFilename, errorMessage, apiVersion);
        }
    }

    if (errorMessage) {
        char* msgbuf = (char*)malloc(1024);
        snprintf(msgbuf, 1024, "cwipc_capturer: auto: camera type \"%s\" not supported", type.c_str());
        *errorMessage = msgbuf;
    }

    return nullptr;
}

int _cwipc_register_capturer(const char *name, _cwipc_functype_count_devices *countFunc, _cwipc_func_capturer_factory *factoryFunc) {
    struct capturer new_capturer;

    new_capturer.name = name;
    new_capturer.countFunc = countFunc;
    new_capturer.factoryFunc = factoryFunc;
    all_capturers.push_back(new_capturer);

    return 1;
}
