#ifndef cwipc_kinect_api_h
#define cwipc_kinect_api_h
/** @file cwipc_kinect/api.h
 * Header for cwipc Kinect capturer, including both C and C++ interfaces.
 *
 * This header is intended to be the only header needed to use the cwipc Kinect capturer.
 */

#include "cwipc_util/api.h"

/* Ensure we have the right dllexport or dllimport on windows */
#ifndef _CWIPC_KINECT_EXPORT
#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_KINECT_EXPORT __declspec(dllimport)
#else
#define _CWIPC_KINECT_EXPORT 
#endif
#endif

#ifdef __cplusplus
extern "C" {
#endif


/** \brief
 * Returns version string for realsense2 capturer module.
 * As a side effect this checks that dependencies have been installed correctly, and it will install the
 * capturer for use with the auto-capturer.
 */
_CWIPC_KINECT_EXPORT const char *cwipc_get_version_kinect();

/** \brief Capture pointclouds from kinect cameras.
 * \param configFilename An option string with the filename of the camera configuration file.
 * \param errorMessage An optional pointer to a string where any error message will be stored.
 * \param apiVersion Pass in CWIPC_API_VERSION to ensure DLL compatibility.
 * \return A cwipc_source object.

 * This function returns a cwipc_source that captures pointclouds from realsense
 * cameras. If no camera is connected it will return "watermelon" pointclouds
 * similar to the `cwipc_synthetic()` source.
 */

_CWIPC_KINECT_EXPORT cwipc_activesource* cwipc_kinect(const char *configFilename, char **errorMessage, uint64_t apiVersion);



/** \brief Capture pointclouds from k4arecordings.
 * \param configFilename An option string with the filename of the camera configuration file.
 * \param errorMessage An optional pointer to a string where any error message will be stored.
 * \param apiVersion Pass in CWIPC_API_VERSION to ensure DLL compatibility.
 * \return A cwipc_activesource object.

 * This function returns a cwipc_source that create pointclouds from color and
 * depth images captured earlier (or elsewhere) from realsense
 * cameras.
 */

_CWIPC_KINECT_EXPORT cwipc_activesource* cwipc_kinect_playback(const char* configFilename, char** errorMessage, uint64_t apiVersion);

#ifdef __cplusplus
}
#endif


#endif /* cwipc_kinect_api_h */
