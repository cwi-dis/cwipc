import sys
import time
import cwipc

file = sys.argv[1]
count = int(sys.argv[2])
cellsize = float(sys.argv[3])
print(f"Loading {file}")
pc = cwipc.cwipc_read(file, 0)
t0 = time.time()
pccount = 0
for i in range(count):
    pc2 = cwipc.cwipc_downsample(pc, cellsize)
    pccount = pc2.count()
    pc2.free()
t1 = time.time()
dt = t1 - t0
print(f"Downsampled {count} times to {cellsize} in {dt:.3f} seconds, {dt/count:.3f} s each, {pccount} points")