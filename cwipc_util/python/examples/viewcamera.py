import sys
import os
import cwipc

def main():
    if len(sys.argv) > 2:
        print(f"Usage: {sys.argv[0]} [cameraconfig]", file=sys.stderr)
        print("Capture pointclouds and show them in a window", file=sys.stderr)
        print("Optional argument can be filename of a cameraconfig.json file or auto.", file=sys.stderr)
        print("Default: use ./cameraconfig.json", file=sys.stderr)
        sys.exit(2)
    
    config = None
    if len(sys.argv) > 1:
        config = sys.argv[1]

    generator = cwipc.cwipc_capturer(config)
    generator.start()
    
    sink = cwipc.cwipc_window(sys.argv[0])

    ok = True
    while ok:
        if generator.available(False):
            pc = generator.get()
            assert pc
            ok = sink.feed(pc, True)
        else:
            # If no pointcloud available we still call feed()
            # to allow user interaction
            ok = sink.feed(None, False)

if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
    