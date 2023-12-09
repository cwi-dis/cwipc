import sys
import os
import math
from typing import Optional, List, Any, Tuple
import numpy as np
import scipy.spatial
from matplotlib import pyplot as plt
import cwipc
import cwipc.filters.colorize
import cwipc.registration.multicamera


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

    fixer = cwipc.registration.multicamera.MultiCamera()
    # This number sets a threashold for the best possible alignment.
    # xxxjack it should be computed from the source point clouds
    original_capture_precision = 0.001

    fixer.verbose = True
    fixer.show_plot = True
    fixer.add_tiled_pointcloud(pc)
    fixer.run()
    
if __name__ == '__main__':
    main()