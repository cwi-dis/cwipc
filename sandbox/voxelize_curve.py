import sys
import cwipc

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} plyfile", file=sys.stderr)
        sys.exit(1)
    pc = cwipc.cwipc_read(sys.argv[1], 0)
    originalCount = pc.count()
    originalCellsize = pc.cellsize()
    print("iteration,cellSize,pointCount,increaseFactor,perTileCountCount")
    print(f"0,{originalCellsize},{originalCount},{0},,")
    newCount = -1
    cellSize = 1
    iteration = 1
    oldCount = 0
    while cellSize > originalCellsize:
        try:
            newPc = cwipc.cwipc_downsample(pc, cellSize)
        except cwipc.CwipcError as e:
            print(f"cwipc_downsample: Exception {e}", file=sys.stderr)
            break
        newCount = newPc.count()
        if oldCount == 0:
            increaseFactor = 0
        else:
            increaseFactor = newCount / oldCount
        oldCount = newCount
        histogram = tileHistogram(newPc)
        histCombined = histogramCombine(histogram)
        print(f"{iteration},{cellSize},{newCount},{increaseFactor},{histCombined}")
        cwipc.cwipc_write(f"downsample-{iteration}.ply", newPc)
        newPc.free()
        cellSize = cellSize * 0.7071
        iteration += 1

def tileHistogram(pc : cwipc.cwipc_wrapper) -> list[int]:
    """For a given point cloud, return the histogram how many points are in each tile"""
    points = pc.get_points()
    maxTile = 0
    p : cwipc.cwipc_point
    for p  in points:
        if p.tile > maxTile:
            maxTile = p.tile
    rv = [0] * (maxTile+1)
    for p in points:
        rv[p.tile] += 1
    return rv

def histogramCombine(counts : list[int]) -> list[int]:
    maxBitCount = 0
    for tileMask in range(len(counts)):
        if counts[tileMask] > 0:
            bitCount = countBits(tileMask)
            if bitCount > maxBitCount:
                maxBitCount = bitCount
    rv = [0]*(maxBitCount+1)
    for tileMask in range(len(counts)):
        if counts[tileMask] > 0:
            bitCount = countBits(tileMask)
            rv[bitCount] = rv[bitCount] + counts[tileMask]
    return rv

def countBits(src : int) -> int:
    """Return number of bits set in argument"""
    return bin(src).count('1')

if __name__ == '__main__':
    main()