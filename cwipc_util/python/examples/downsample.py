import sys
import os
import cwipc

def main():
    if len(sys.argv) != 4:
        print(f"Usage: {sys.argv[0]} voxelsize pointcloudfile.ply newpointcloudfile.ply", file=sys.stderr)
        sys.exit(2)
    voxelsize = float(sys.argv[1])

    pc = cwipc.cwipc_read(sys.argv[2], 0)

    new_pc = cwipc.cwipc_downsample(pc, voxelsize)

    cwipc.cwipc_write(sys.argv[3], new_pc)


if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
