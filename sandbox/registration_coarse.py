import sys
import os
import math
from typing import Optional, List, Any, Tuple
import numpy as np
import scipy.spatial
from matplotlib import pyplot as plt
import cwipc
import cwipc.filters.colorize
import cwipc.registration.analyze
import cwipc.registration.compute
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
    fixer = cwipc.registration.coarse.MultiCameraCoarseInteractive()
    fixer.add_tiled_pointcloud(pc)
    ok = fixer.run()
    if not ok:
        print("Could not do coarse registration")
        sys.exit(1)
    # Get the result
    transformations = fixer.get_result_transformations()
    for i in range(len(transformations)):
        camnum = fixer.tilenum_for_camera_index(i)
        print(f"cam-index {i}: camnum {camnum}: matrix {transformations[i]}")
    new_pc = fixer.get_result_pointcloud_full()
   # _ = run_analyzer(new_pc, 1.0, basefilename, "", "", True)
    cwipc.registration.util.show_pointcloud("Result", new_pc)

def run_analyzer(pc : cwipc.cwipc_wrapper, original_capture_precision : float, basefilename : str, png_filename : str, extlabel : str, plot : bool) -> Tuple[Optional[int], float]:
    analyzer = cwipc.registration.analyze.RegistrationAnalyzerOneToAll()
    analyzer.add_tiled_pointcloud(pc)
    analyzer.label = basefilename + extlabel
    analyzer.run()
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