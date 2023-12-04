import sys
import os
import math
from typing import Optional, List, Any
import numpy as np
import scipy.spatial
from matplotlib import pyplot as plt
import cwipc
import cwipc.filters.colorize
import cwipc.registration.analyze
import cwipc.registration.compute


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
    camnum_to_fix, correspondence = run_analyzer(pc, basefilename, png_filename, " before", True)
    
    step = 1
    while True:
        png_filename = basefilename + f"_histogram_after_step_{step}.png"
        new_pc = run_fixer(pc, camnum_to_fix, correspondence)
        pc.free()
        pc = new_pc
        old_camnum_to_fix, old_correspondence = camnum_to_fix, correspondence
        
        camnum_to_fix, correspondence = run_analyzer(pc, basefilename, png_filename, f" after {step}", True)
        if camnum_to_fix == old_camnum_to_fix and correspondence >= old_correspondence:
            print(f"No more improvement. Camera {old_camnum_to_fix} was at {old_correspondence}, now camera {camnum_to_fix} is at {correspondence}")
            break
        step += 1
    ply_filename = basefilename + "_after.ply"
    cwipc.cwipc_write(ply_filename, pc)

def run_analyzer(pc : cwipc.cwipc_wrapper, basefilename : str, png_filename : str, extlabel : str, plot : bool):
    analyzer = cwipc.registration.analyze.RegistrationAnalyzerOneToAll()
    analyzer.add_tiled_pointcloud(pc)
    analyzer.label = basefilename + extlabel
    analyzer.want_histogram_plot = True
    analyzer.run()
    results = analyzer.get_ordered_results()
    print(f"Sorted correspondences {extlabel}")
    for camnum, correspondence, weight in results:
        print(f"\tcamnum={camnum}, correspondence={correspondence}, weight={weight}")
    camnum_to_fix = results[0][0]
    correspondence = results[0][1]
    if plot:
        analyzer.save_plot(png_filename, True)
    return camnum_to_fix, correspondence

def run_fixer(pc : cwipc.cwipc_wrapper, camnum_to_fix : int, correspondence : float) -> cwipc.cwipc_wrapper:
    computer = cwipc.registration.compute.RegistrationComputer_ICP()
    print(f"Will fix camera {camnum_to_fix}, correspondence={correspondence}, algorithm={computer.__class__.__name__}")
    computer.add_tiled_pointcloud(pc)
    computer.set_correspondence(correspondence)
    computer.run(camnum_to_fix)
    transform = computer.get_result_transformation()
    print(f"Transformation={transform}")
    new_pc = computer.get_result_pointcloud_full()
    return new_pc

if __name__ == '__main__':
    main()