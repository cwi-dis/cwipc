import sys
import os
import cwipc

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} pointcloudfile.ply pointcloudfile.cwipcdump", file=sys.stderr)
        sys.exit(2)

    pc = cwipc.cwipc_read(sys.argv[1], 0)

    cwipc.cwipc_write_debugdump(sys.argv[2], pc)

if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
    