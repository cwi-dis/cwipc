import sys
import os
import math
import numpy as np
import scipy.spatial
from matplotlib import pyplot as plt
import cwipc
import cwipc.filters.colorize

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} plyfile", file=sys.stderr)
        sys.exit(1)
    basefilename, ext = os.path.splitext(sys.argv[1])
    csv_filename = basefilename + ".csv"
    png_filename = basefilename + "_cumdist.png"

    if ext.lower() == '.ply':
        pc = cwipc.cwipc_read(sys.argv[1], 0)
    else:
        pc = cwipc.cwipc_read_debugdump(sys.argv[1])
    pc_cam1 = cwipc.cwipc_tilefilter(pc, 1)
    np_cam1 = np.ctypeslib.as_array(pc_cam1.get_points())
    pc_cam2 = cwipc.cwipc_tilefilter(pc, 2)
    np_cam2 = np.ctypeslib.as_array(pc_cam2.get_points())
    pc_cam4 = cwipc.cwipc_tilefilter(pc, 4)
    np_cam4 = np.ctypeslib.as_array(pc_cam4.get_points())
    pc_cam8 = cwipc.cwipc_tilefilter(pc, 8)
    np_cam8 = np.ctypeslib.as_array(pc_cam8.get_points())

    xyz_cam1 = np_cam1[['x','y','z']]
    xyz_cam2 = np_cam2[['x','y','z']]
    xyz_cam4 = np_cam4[['x','y','z']]
    xyz_cam8 = np_cam8[['x','y','z']]


    points_cam1 = np.column_stack([xyz_cam1['x'],xyz_cam1['y'],xyz_cam1['z']])
    points_cam2 = np.column_stack([xyz_cam2['x'],xyz_cam2['y'],xyz_cam2['z']])
    points_cam4 = np.column_stack([xyz_cam4['x'],xyz_cam4['y'],xyz_cam4['z']])
    points_cam8 = np.column_stack([xyz_cam8['x'],xyz_cam8['y'],xyz_cam8['z']])

    tree_cam1 = scipy.spatial.KDTree(points_cam1)
    tree_cam2 = scipy.spatial.KDTree(points_cam2)
    tree_cam4 = scipy.spatial.KDTree(points_cam4)
    tree_cam8 = scipy.spatial.KDTree(points_cam8)

    dists_1to2, _ = tree_cam2.query(points_cam1)
    dists_1to4, _ = tree_cam4.query(points_cam1)
    dists_1to8, _ = tree_cam8.query(points_cam1)
    dists_2to4, _ = tree_cam4.query(points_cam2)
    dists_2to8, _ = tree_cam8.query(points_cam2)
    dists_4to8, _ = tree_cam8.query(points_cam4)

    histogram_1to2, edges_1to2 = np.histogram(dists_1to2, bins=400)
    cumsum_1to2 = np.cumsum(histogram_1to2)
    histogram_1to4, edges_1to4 = np.histogram(dists_1to4, bins=400)
    cumsum_1to4 = np.cumsum(histogram_1to4)
    histogram_1to8, edges_1to8 = np.histogram(dists_1to8, bins=400)
    cumsum_1to8 = np.cumsum(histogram_1to8)
    histogram_2to4, edges_2to4 = np.histogram(dists_2to4, bins=400)
    cumsum_2to4 = np.cumsum(histogram_2to4)
    histogram_2to8, edges_2to8 = np.histogram(dists_2to8, bins=400)
    cumsum_2to8 = np.cumsum(histogram_2to8)
    histogram_4to8, edges_4to8 = np.histogram(dists_4to8, bins=400)
    cumsum_4to8 = np.cumsum(histogram_4to8)

    plt.plot(edges_1to2[1:], cumsum_1to2, label="1-2")
    plt.plot(edges_1to4[1:], cumsum_1to4, label="1-4")
    plt.plot(edges_1to8[1:], cumsum_1to8, label="1-8")
    plt.plot(edges_2to4[1:], cumsum_2to4, label="2-4")
    plt.plot(edges_2to8[1:], cumsum_2to8, label="2-8")
    plt.plot(edges_4to8[1:], cumsum_4to8, label="4-8")
    plt.title("Cumulative point distances between all cameras")
    plt.legend()
    plt.savefig(png_filename)
    plt.show()

def foo():



    
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
    increaseFactor = 0
    tile_color_filter = cwipc.filters.colorize.ColorizeFilter(1, "contributions")

    while cellSize > originalCellsize and cellSize > MAGIC_CELLSIZE_VALUE and newCount < originalCount * POINTCOUNT_END_FACTOR:
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
        cellSize = cellSize * CELLSIZE_NEXT_FACTOR
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