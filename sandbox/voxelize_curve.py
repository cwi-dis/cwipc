import sys
import os
import cwipc
import cwipc.filters.colorize

# At a cellsize of approximately this size voxelizing will stop working,
# because the integers used as indices would overflow. Unsure whether this is actually a fixed number.
MAGIC_CELLSIZE_VALUE = 0.0005

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} plyfile", file=sys.stderr)
        sys.exit(1)
    basefilename, ext = os.path.splitext(sys.argv[1])
    if ext.lower() == '.ply':
        pc = cwipc.cwipc_read(sys.argv[1], 0)
    else:
        pc = cwipc.cwipc_read_debugdump(sys.argv[1])
    originalCount = pc.count()
    originalCellsize = pc.cellsize()
    originalHistogram = tileHistogram(pc)
    originalHistCombined = histogramCombine(originalHistogram)
    print(f"original: {originalCount} points, {len(originalHistogram)} tiles")
    for i in range(len(originalHistogram)):
        if originalHistogram[i]:
            print(f"original: tile {i}: {originalHistogram[i]} points")
    for i in range(len(originalHistCombined)):
        print(f"original: {i}-camera tiles: {originalHistCombined[i]} points")
    p0, p1, p2, p3, p4, _, _, _, _ = originalHistCombined
       
    csv_filename = basefilename + ".csv"
    csv_file = open(csv_filename, "w")
    print("iteration,cellSize,pointCount,increaseFactor,nCam1Count,nCam2Count,nCam3Count,nCam4Count,plyFile", file=csv_file)
    print(f"0,{originalCellsize},{originalCount},,{p1},{p2},{p3},{p4}", file=csv_file)
    newCount = -1
    cellSize = 1
    iteration = 1
    oldCount = 0
    
    tile_color_filter = cwipc.filters.colorize.ColorizeFilter(1, "contributions")

    while cellSize > originalCellsize and cellSize > MAGIC_CELLSIZE_VALUE:
        try:
            newPc = cwipc.cwipc_downsample(pc, cellSize)
        except cwipc.CwipcError as e:
            print(f"cwipc_downsample: Exception {e}", file=sys.stderr)
            break
        try:
            newCount = newPc.count()
        except AssertionError:
            print(f"iteration {iteration}: failed")
            break
        print(f"iteration {iteration}: {newCount} points")
        if oldCount == 0:
            increaseFactor = 0
        else:
            increaseFactor = newCount / oldCount
        oldCount = newCount
        histogram = tileHistogram(newPc)
        histCombined = histogramCombine(histogram)
        p0, p1, p2, p3, p4, _, _, _, _ = histCombined
        tmpPathname = f"{basefilename}-{iteration}.ply"
        _, tmpFilename = os.path.split(tmpPathname)
        print(f"{iteration},{cellSize},{newCount},{increaseFactor},{p1},{p2},{p3},{p4},{tmpFilename}", file=csv_file)
        cwipc.cwipc_write(tmpPathname, newPc)
        newColoredPc = tile_color_filter.filter(newPc)
        tmpColoredPathname = f"{basefilename}-{iteration}-ncam.ply"
        cwipc.cwipc_write(tmpColoredPathname, newColoredPc)
        newPc.free()
        newColoredPc.free()
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
    maxBitCount = 8
    if False:
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