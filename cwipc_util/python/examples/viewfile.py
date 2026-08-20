import sys
import os
import cwipc

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} file", file=sys.stderr)
        print("Read pointcloud from a file and show it in a window", file=sys.stderr)
       
        sys.exit(2)
    
    filename = sys.argv[1]
    if filename.endswith(".ply"):
        pc = cwipc.cwipc_read(filename, 0)
    elif filename.endswith(".cwipcdump"):
        pc = cwipc.cwipc_read_debugdump(filename)
    else:
        print("Filename must be .ply or .cwipcdump")
        sys.exit(2)

    sink = cwipc.cwipc_window(sys.argv[0])
    sink.feed(pc, True)
    ok = True
    while ok:
        ok = sink.feed(None, False)

if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
    