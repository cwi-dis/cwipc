"""
Join multiple point clouds into a single point cloud.
"""
import sys
import os
import time
import shutil
import argparse
import json
from typing import Optional, List, cast, Tuple, Dict
import numpy
import cwipc

def main():
    assert __doc__ is not None
    parser = argparse.ArgumentParser(description=__doc__.strip())
    parser.add_argument("output", help="Output point cloud file, as .ply or .cwipc")
    parser.add_argument("input", nargs='+', help="Input point cloud files, as .ply or .cwipc")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--debugpy", action="store_true", help="Wait for debugpy client to attach")
    args = parser.parse_args()
    if args.debugpy:
        import debugpy
        debugpy.listen(5678)
        print(f"{sys.argv[0]}: waiting for debugpy attach on 5678", flush=True)
        debugpy.wait_for_client()
        print(f"{sys.argv[0]}: debugger attached")
    result : Optional[cwipc.cwipc_pointcloud_wrapper] = None
    for input_file in args.input:
        if input_file.endswith('.ply'):
            pc = cwipc.cwipc_read(input_file, 0)
        elif input_file.endswith('.cwipc'):
            pc = cwipc.cwipc_read_debugdump(input_file)
        else:
            print(f"Unknown file format for {input_file}, expected .ply or .cwipc")
            return 1
        if args.verbose:
            print(f"Loaded point cloud from {input_file}, count: {pc.count()}")
        if result is None:
            result = pc
        else:
            result = cwipc.cwipc_join(result, pc)
        if args.verbose:
            print(f"Joined point cloud, new count: {result.count()}")
    assert result
    if args.output.endswith('.ply'):
        cwipc.cwipc_write(args.output, result)
    elif args.output.endswith('.cwipc'):
        cwipc.cwipc_write_debugdump(args.output, result)
    else:
        print(f"Unknown output file format {args.output}, expected .ply or .cwipc")
        return 1
    return 0

if __name__ == '__main__':
    sys.exit(main())