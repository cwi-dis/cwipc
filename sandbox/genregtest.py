import sys
import argparse
from typing import List, Tuple
import cwipc
import cwipc.filters.transform

MyPoint = Tuple[float, float, float]
MyBoundingBox = Tuple[float, float, float, float, float, float]

BBOX_0 = (-10, -0.1, 0, 3, -10, 10)   # Camera 0: negative X direction
BBOX_1 = (-10, 10, 0, 3, -10, -0.1)   # Camera 1: negative Z direction
BBOX_2 = (0.1, 10, 0, 3, -10, 10)     # Camera 2: positive X direction
BBOX_3 = (-10, 10, 0, 3, 0.1, 10)     # Camera 3: positive Z direction

def main():
    parser = argparse.ArgumentParser(sys.argv[0], description="Create a PLY point cloud file that can be used to test registration algorithms")
    parser.add_argument("--distance", action="store", type=float, default="0.01", metavar="DIST", help="Camera 2 and 4 point clouds will be move DIST meters out from where they should be")
    parser.add_argument("--npoint", action="store", type=int, default=200000, metavar="COUNT", help="Create original point cloud of approximately this many points")
    parser.add_argument("--two", action="store_true", help="Output only camera 1 and 2 points (in stead of all 4)")
    parser.add_argument("--single", action="store_true", help="Modify only camera 2 (in stead of 2 and 4)")
    parser.add_argument("output", action="store", help="Output ply file")
    args = parser.parse_args()
    distance = args.distance
    npoint = args.npoint
    output = args.output

    offsets = [(0,0,0), (0,0,-distance), (0,0,0), (0,0,distance)]
    if args.single:
        offsets[3] = (0, 0, 0)
    pc = create_regtest_pointcloud(npoint, offsets, args.two)
    cwipc.cwipc_write(output, pc)

def create_regtest_pointcloud(npoint : int, offsets : List[MyPoint], twocam : bool=False) -> cwipc.cwipc_wrapper:
    gen = cwipc.cwipc_synthetic(npoints=npoint)
    pc = gen.get()
    assert pc
    pc0 = construct_partial_pc(pc, BBOX_0, offsets[0], 1)
    pc1 = construct_partial_pc(pc, BBOX_1, offsets[1], 2)
    pc2 = construct_partial_pc(pc, BBOX_2, offsets[2], 4)
    pc3 = construct_partial_pc(pc, BBOX_3, offsets[3], 8)
    pc.free()
    #
    tmp01 = cwipc.cwipc_join(pc0, pc1)
    if twocam:
        rv = tmp01
    else:
        tmp23 = cwipc.cwipc_join(pc2, pc3)
        rv = cwipc.cwipc_join(tmp01, tmp23)
        tmp01.free()
        tmp23.free()
    pc0.free()
    pc1.free()
    pc2.free()
    pc3.free()
    return rv

def construct_partial_pc(pc : cwipc.cwipc_wrapper, bbox : MyBoundingBox, offset : MyPoint, tilemask : int) -> cwipc.cwipc_wrapper:
    cropped_pc = cwipc.cwipc_crop(pc, bbox)
    x, y, z = offset
    filter = cwipc.filters.transform.TransformFilter(x, y, z, 1.0)
    moved_pc = filter.filter(cropped_pc)
    cropped_pc.free()
    tilemap = [tilemask]*256
    tilemapped_pc = cwipc.cwipc_tilemap(moved_pc, tilemap)
    moved_pc.free()
    return tilemapped_pc

if __name__ == '__main__':
    main()