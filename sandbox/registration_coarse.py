import sys
import os
import time
import math
from typing import Optional, List, Any, Tuple
import numpy as np
import scipy.spatial
from matplotlib import pyplot as plt
import cwipc
import cwipc.filters.colorize
import cwipc.registration.analyze
import cwipc.registration.fine
import cwipc.registration.coarse
import cwipc.registration.util


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} plyfile", file=sys.stderr)
        sys.exit(1)
    basefilename, ext = os.path.splitext(sys.argv[1])
    if ext.lower() == '.ply':
        pc = cwipc.cwipc_read(sys.argv[1], 0)
    else:
        pc = cwipc.cwipc_read_debugdump(sys.argv[1])
    # Run the coarse calibration
    pointcloud = True
    if not pointcloud:
        aligner = cwipc.registration.coarse.MultiCameraCoarseColorTarget()
    else:
        aligner = cwipc.registration.coarse.MultiCameraCoarseAruco()
    aligner.debug = True
    aligner.verbose = True
    aligner.add_tiled_pointcloud(pc)
    start_time = time.time()
    ok = aligner.run()
    stop_time = time.time()
    print(f"aligner ran for {stop_time-start_time:.3f} seconds")
    if not ok:
        print("Could not do coarse registration")
        sys.exit(1)
    # Get the result
    transformations = aligner.get_result_transformations()
    for i in range(len(transformations)):
        camnum = aligner.tilemask_for_camera_index(i)
        is_identity = (transformations[i] == cwipc.registration.util.transformation_identity()).all()
        print(f"cam-index {i}: camnum {camnum}: registered={not is_identity}, matrix {transformations[i]}")
    start_time = time.time()
    new_pc = aligner.get_result_pointcloud_full()
    stop_time = time.time()
    print(f"transformer ran for {stop_time-start_time:.3f} seconds")
    cwipc.registration.util.show_pointcloud("Result for all cameras in 3D. ESC to close.", new_pc)
    ply_filename = basefilename + "_after.ply"
    cwipc.cwipc_write(ply_filename, new_pc)
    pngfilename = basefilename + ".png"
    # This is very expensive and not very helpful._ = run_analyzer(new_pc, 1.0, basefilename, pngfilename, "", True)
    
def run_analyzer(pc : cwipc.cwipc_wrapper, original_capture_precision : float, basefilename : str, png_filename : str, extlabel : str, plot : bool) -> Tuple[Optional[int], float]:
    analyzer = cwipc.registration.analyze.RegistrationAnalyzer()
    analyzer.add_tiled_pointcloud(pc)
    analyzer.plot_label = basefilename + extlabel
    start_time = time.time()
    analyzer.run()
    stop_time = time.time()
    print(f"analyzer ran for {stop_time-start_time:.3f} seconds")
    results = analyzer.get_ordered_results()
    print(f"Sorted correspondences {extlabel}")
    for camnum, correspondence, weight in results:
        print(f"\tcamnum={camnum}, correspondence={correspondence}, weight={weight}")
    camnum_to_fix = None
    correspondence = results[0][1]
    for i in range(len(results)):
        if results[i][1] > original_capture_precision:
            camnum_to_fix = results[i][0]
            correspondence = results[i][1]
            break
    if plot:
        analyzer.plot(filename=png_filename, show=True)
    return camnum_to_fix, correspondence

if __name__ == '__main__':
    main()