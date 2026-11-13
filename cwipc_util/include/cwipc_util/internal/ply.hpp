#ifndef _cwipc_util_internal_ply_hpp_
#define _cwipc_util_internal_ply_hpp_

#include "cwipc_util/api.h"

/** \brief Write a point cloud as a PLY file (#66).
 *
 * Writes the points with properties in struct cwipc_point order (x, y, z as float;
 * red, green, blue, tile as uchar), followed by an element cwipc_metadata with the
 * timestamp (in seconds, as double) and the cellsize (float).
 * Binary files are little-endian, and their header is padded to a multiple of 16
 * bytes, so the point data that follows it has exactly the cwipc_point layout.
 *
 * \param filename Output file.
 * \param pc The point cloud.
 * \param binary Write binary (true) or ASCII (false).
 * \return 0 on success, -1 on failure (after logging an error with cwipc_log).
 */
int _cwipc_ply_write(const char* filename, cwipc_pointcloud* pc, bool binary);

/** \brief Read a point cloud from a PLY file (#66).
 *
 * Accepts ASCII or binary PLY with a vertex element that has x, y, z (any numeric type),
 * optionally uchar red/green/blue and the fourth byte as uchar tile or alpha; other
 * properties and elements are skipped. Colours that can't be used give a log warning
 * and black points. An element cwipc_metadata provides timestamp and cellsize.
 *
 * \param filename Input file.
 * \param timestamp Timestamp for the point cloud. If 0, the timestamp from the file's
 *   cwipc_metadata is used (if present).
 * \param errorMessage Passed on to cwipc_from_points.
 * \return The point cloud, or NULL on failure (after logging an error with cwipc_log).
 */
cwipc_pointcloud* _cwipc_ply_read(const char* filename, uint64_t timestamp, char** errorMessage);

#endif // _cwipc_util_internal_ply_hpp_
