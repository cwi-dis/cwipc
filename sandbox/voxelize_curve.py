import sys
import cwipc

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} plyfile", file=sys.stderr)
        sys.exit(1)
    pc = cwipc.cwipc_read(sys.argv[1], 0)
    originalCount = pc.count()
    originalCellsize = pc.cellsize()
    print("iteration,cellSize,pointCount")
    print(f"0,{originalCellsize},{originalCount}")
    newCount = -1
    cellSize = 1
    iteration = 1
    while cellSize > originalCellsize:
        try:
            newPc = cwipc.cwipc_downsample(pc, cellSize)
        except cwipc.CwipcError as e:
            print(f"cwipc_downsample: Exception {e}", file=sys.stderr)
            break
        newCount = newPc.count()
        print(f"{iteration},{cellSize},{newCount}")
        newPc.free()
        cellSize = cellSize * 0.7071
        iteration += 1

if __name__ == '__main__':
    main()