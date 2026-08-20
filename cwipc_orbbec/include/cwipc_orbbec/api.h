#ifndef cwipc_orbbec_api_h
#define cwipc_orbbec_api_h
/** @file cwipc_orbbec/api.h
 * Header for cwipc Orbbec capturer, including both C and C++ interfaces.
 *
 * This header is intended to be the only header needed to use the cwipc Orbbec capturer.
 */

#include "cwipc_util/api.h"

/* Ensure we have the right dllexport or dllimport on windows */
#ifndef _CWIPC_ORBBEC_EXPORT
#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_ORBBEC_EXPORT __declspec(dllimport)
#else
#define _CWIPC_ORBBEC_EXPORT 
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif
/** \brief
 * Returns version string for orbbec capturer module.
 * As a side effect this checks that dependencies have been installed correctly, and it will install the
 * capturer for use with the auto-capturer.
 */
_CWIPC_ORBBEC_EXPORT const char *cwipc_get_version_orbbec();

/** \brief Capture pointclouds from Orbbec cameras.
 * \param configFilename An option string with the filename of the camera configuration file.
 * \param errorMessage An optional pointer to a string where any error message will be stored.
 * \param apiVersion Pass in CWIPC_API_VERSION to ensure DLL compatibility.
 * \return A cwipc_source object.

 * This function returns a cwipc_source that captures pointclouds from realsense
 * cameras. If no camera is connected it will return "watermelon" pointclouds
 * similar to the `cwipc_synthetic()` source.
 */

_CWIPC_ORBBEC_EXPORT cwipc_activesource* cwipc_orbbec(const char *configFilename, char **errorMessage, uint64_t apiVersion);



/** \brief Capture pointclouds from Orbbec recordings.
 * \param configFilename An option string with the filename of the camera configuration file.
 * \param errorMessage An optional pointer to a string where any error message will be stored.
 * \param apiVersion Pass in CWIPC_API_VERSION to ensure DLL compatibility.
 * \return A cwipc_activesource object.

 * This function returns a cwipc_source that create pointclouds from color and
 * depth images captured earlier (or elsewhere) from orbbec
 * cameras.
 */

_CWIPC_ORBBEC_EXPORT cwipc_activesource* cwipc_orbbec_playback(const char* configFilename, char** errorMessage, uint64_t apiVersion);


#ifdef __cplusplus
}
#endif


#endif /* cwipc_orbbec_api_h */
