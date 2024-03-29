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


def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} plyfile", file=sys.stderr)
        sys.exit(1)
    basefilename, ext = os.path.splitext(sys.argv[1])
    csv_filename = basefilename + ".csv"
    png_filename = basefilename + "_histogram_one2all.png"

    if ext.lower() == '.ply':
        pc = cwipc.cwipc_read(sys.argv[1], 0)
    else:
        pc = cwipc.cwipc_read_debugdump(sys.argv[1])

    # This number sets a threashold for the best possible alignment.
    # xxxjack it should be computed from the source point clouds
    original_capture_precision = 0.001

    camnum_to_fix, correspondence_error = run_analyzer(pc, original_capture_precision, basefilename, png_filename, " before", True)
    
    
    step = 1
    while camnum_to_fix != None:
        png_filename = basefilename + f"_histogram_after_step_{step}.png"
        new_pc = run_aligner(pc, camnum_to_fix, correspondence_error)
        pc.free()
        pc = new_pc
        old_camnum_to_fix, old_correspondence_error = camnum_to_fix, correspondence_error
        
        camnum_to_fix, correspondence_error = run_analyzer(pc, original_capture_precision, basefilename, png_filename, f" after {step}", True)
        if camnum_to_fix == old_camnum_to_fix and correspondence_error >= old_correspondence_error:
            print(f"No more improvement. Camera {old_camnum_to_fix} was at {old_correspondence_error}, now camera {camnum_to_fix} is at {correspondence_error}")
            break
        step += 1
    if camnum_to_fix == None:
        print(f"All cameras have correspondence below capture precision of {original_capture_precision}")
    ply_filename = basefilename + "_after.ply"
    cwipc.cwipc_write(ply_filename, pc)

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

def run_aligner(pc : cwipc.cwipc_wrapper, camnum_to_fix : int, correspondence : float) -> cwipc.cwipc_wrapper:
    aligner = cwipc.registration.fine.RegistrationComputer_ICP_Point2Point()
    print(f"Will fix camera {camnum_to_fix}, correspondence={correspondence}, algorithm={aligner.__class__.__name__}")
    aligner.add_tiled_pointcloud(pc)
    aligner.set_correspondence(correspondence)
    start_time = time.time()
    aligner.run(camnum_to_fix)
    stop_time = time.time()
    print(f"aligner ran for {stop_time-start_time:.3f} seconds")
    transform = aligner.get_result_transformation()
    print(f"Transformation={transform}")
    start_time = time.time()
    new_pc = aligner.get_result_pointcloud_full()
    stop_time = time.time()
    print(f"transformer ran for {stop_time-start_time:.3f} seconds")
    return new_pc

if __name__ == '__main__':
    main()