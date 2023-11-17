import sys
import os
import math
from typing import Optional, List, Any
import numpy as np
import scipy.spatial
from matplotlib import pyplot as plt
import cwipc
import cwipc.filters.colorize


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} plyfile", file=sys.stderr)
        sys.exit(1)
    basefilename, ext = os.path.splitext(sys.argv[1])
    csv_filename = basefilename + ".csv"
    png_filename = basefilename + "_cumdist.png"

    if ext.lower() == '.ply':
        pc = cwipc.cwipc_read(sys.argv[1], 0)
    else:
        pc = cwipc.cwipc_read_debugdump(sys.argv[1])

    # Get per-camera pointclouds (cwipc_wrpper)
    per_camera_pointclouds = [
        get_pc_for_cam(pc, tilemask) for tilemask in [1,2,4,8,16,32,64,128]
    ]
    # Get per-camera 2-dimensional point arrays (X,Y,Z), only for cameras that exist and have any points.
    per_camera_points_nparray = [
        get_nparray_for_pc(cam_pc) for cam_pc in per_camera_pointclouds if cam_pc != None
    ]
    # Create the corresponding kdtrees
    per_camera_kdtree = [
        scipy.spatial.KDTree(points) for points in per_camera_points_nparray
    ]
    nCamera = len(per_camera_kdtree)

    inter_camera_histograms : List[List[Any]] = [[None] * nCamera] * nCamera
    for cam_i in range(nCamera):
        for cam_j in range(cam_i+1, nCamera):
            distances, _ = per_camera_kdtree[cam_j].query(per_camera_points_nparray[cam_i])
            histogram, edges = np.histogram(distances, bins=400)
            cumsum = np.cumsum(histogram)
            inter_camera_histograms[cam_i][cam_j] = (cumsum, edges)
            plt.plot(edges[1:], cumsum, label=f"{cam_i} - {cam_j}")

    plt.title("Cumulative point distances between all cameras")
    plt.legend()
    plt.savefig(png_filename)
    plt.show()


def get_pc_for_cam(pc : cwipc.cwipc_wrapper, tilemask : int) -> Optional[cwipc.cwipc_wrapper]:
    rv = cwipc.cwipc_tilefilter(pc, tilemask)
    if rv.count() != 0:
        return rv
    rv.free()
    return None

def get_nparray_for_pc(pc : cwipc.cwipc_wrapper):
    # Get the points (as a cwipc-style array) and convert them to a NumPy array-of-structs
    pointarray = np.ctypeslib.as_array(pc.get_points())
    # Extract the relevant fields (X, Y, Z coordinates)
    xyzarray = pointarray[['x', 'y', 'z']]
    # Turn this into an N by 3 2-dimensional array
    nparray = np.column_stack([xyzarray['x'], xyzarray['y'], xyzarray['z']])
    return nparray

if __name__ == '__main__':
    main()