import sys
import os
import cwipc

def main():
    if len(sys.argv) > 3:
        print(f"Usage: {sys.argv[0]} [fps [npoints]]", file=sys.stderr)
        print("Create synthetic pointclouds and show them in a window")
        sys.exit(2)
    fps = 0
    npoints = 0
    if len(sys.argv) > 1:
        fps = int(sys.argv[1])
    if len(sys.argv) > 2:
        npoints = int(sys.argv[2])

    generator = cwipc.cwipc_synthetic(fps, npoints)
    generator.start()
    sink = cwipc.cwipc_window(sys.argv[0])

    ok = True
    while ok:
        pc = generator.get()
        assert pc
        ok = sink.feed(pc, True)

if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
    