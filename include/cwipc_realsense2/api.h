#ifndef cwipc_realsense_api_h
#define cwipc_realsense_api_h
/** @file cwipc_realsense/api.h
 * Header for cwipc Realsense2 capturer, including both C and C++ interfaces.
 *
 * This header is intended to be the only header needed to use the cwipc Realsense2 capturer.
 */

#include "cwipc_util/api.h"

/* Ensure we have the right dllexport or dllimport on windows */
#ifndef _CWIPC_REALSENSE2_EXPORT
#if defined(WIN32) || defined(_WIN32)
#define _CWIPC_REALSENSE2_EXPORT __declspec(dllimport)
#else
#define _CWIPC_REALSENSE2_EXPORT 
#endif
#endif


#ifdef __cplusplus
extern "C" {
#endif

extern _CWIPC_REALSENSE2_EXPORT int CWIPC_RS2_FORMAT_RGB8;  //!< Constant: 24-bit RGB pixels
extern _CWIPC_REALSENSE2_EXPORT int CWIPC_RS2_FORMAT_Z16;   //!< Constant: 16-bit Depth pixels

/** \brief
 * Returns version string for realsense2 capturer module.
 * As a side effect this checks that dependencies have been installed correctly, and it will install the
 * capturer for use with the auto-capturer.
 */
_CWIPC_REALSENSE2_EXPORT const char *cwipc_get_version_realsense2();

/** \brief Capture pointclouds from realsense2 cameras.
 * \param configFilename An option string with the filename of the camera configuration file.
 * \param errorMessage An optional pointer to a string where any error message will be stored.
 * \param apiVersion Pass in CWIPC_API_VERSION to ensure DLL compatibility.
 * \return A cwipc_source object.

 * This function returns a cwipc_source that captures pointclouds from realsense
 * cameras. If no camera is connected it will return "watermelon" pointclouds
 * similar to the `cwipc_synthetic()` source.
 */

_CWIPC_REALSENSE2_EXPORT cwipc_activesource* cwipc_realsense2(const char *configFilename, char **errorMessage, uint64_t apiVersion);

/** \brief Capture pointclouds from realsense2 recordings (.bag files).
 * \param configFilename An option string with the filename of the camera-recording configuration file.
 * \param errorMessage An optional pointer to a string where any error message will be stored.
 * \param apiVersion Pass in CWIPC_API_VERSION to ensure DLL compatibility.
 * \return A cwipc_source object.

 * This function returns a cwipc_source that captures pointclouds from realsense
 * `.bag` prerecorded streams. The cameraconfig file should contain the references
 * to the file used for each camera (plus the usual transformation matrices and other parameters).
 */

_CWIPC_REALSENSE2_EXPORT cwipc_activesource* cwipc_realsense2_playback(const char *configFilename, char **errorMessage, uint64_t apiVersion);
#ifdef __cplusplus
};
#endif

#endif /* cwipc_realsense_api_h */
