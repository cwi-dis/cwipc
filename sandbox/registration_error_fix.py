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

    analyzer = cwipc.registration.analyze.RegistrationAnalyzerOneToAll()
    analyzer.add_tiled_pointcloud(pc)
    analyzer.label = basefilename + " before"
    analyzer.want_histogram_plot = True
    analyzer.run()
    analyzer.save_plot(png_filename, True)
    results = analyzer.get_ordered_results()
    for camnum, correspondence, weight in results:
        print(f"camnum={camnum}, correspondence={correspondence}, weight={weight}")

    camnum_to_fix = results[0][0]
    print(f"Will fix camera {camnum_to_fix}")
    computer = cwipc.registration.compute.RegistrationComputer()
    computer.add_tiled_pointcloud(pc)
    computer.run(camnum_to_fix)
    transform = computer.get_result_transformation()
    print(f"Transformation={transform}")
    new_pc = computer.get_result_pointcloud_full()
    cwipc.cwipc_write("tmp_1_fixed.ply", new_pc)
    
    analyzer = cwipc.registration.analyze.RegistrationAnalyzerOneToAll()
    analyzer.add_tiled_pointcloud(new_pc)
    analyzer.label = basefilename + " after"
    analyzer.want_histogram_plot = True
    analyzer.run()
    analyzer.save_plot("", True)
    results = analyzer.get_ordered_results()
    for camnum, correspondence, weight in results:
        print(f"camnum={camnum}, correspondence={correspondence}, weight={weight}")


if __name__ == '__main__':
    main()