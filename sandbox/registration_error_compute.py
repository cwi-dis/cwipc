import sys
import os
import time
import math
from typing import Optional, List, Any
import numpy as np
import scipy.spatial
from matplotlib import pyplot as plt
import cwipc
import cwipc.filters.colorize
import cwipc.registration.analyze


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

    analyzer = cwipc.registration.analyze.RegistrationAnalyzer()
    analyzer.add_tiled_pointcloud(pc)
    analyzer.label = basefilename
    start_time = time.time()
    analyzer.run()
    stop_time = time.time()
    print(f"analyzer ran for {stop_time-start_time:.3f} seconds")
    analyzer.plot(filename=png_filename, show=True)
    results = analyzer.get_ordered_results()
    for camnum, correspondence, weight in results:
        print(f"camnum={camnum}, correspondence={correspondence}, weight={weight}")
    

if __name__ == '__main__':
    main()