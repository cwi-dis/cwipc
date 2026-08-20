import sys
import os
import cwipc

def main():
    if len(sys.argv) != 2:
        print(f"Usage: {sys.argv[0]} directory", file=sys.stderr)
        print("Read pointclouds from a directory and show them in a window", file=sys.stderr)
       
        sys.exit(2)
    
    dirname = sys.argv[1]
    allfiles = os.listdir(dirname)
    plyfiles = filter(lambda fn : fn.endswith(".ply") or fn.endswith(".cwipcdump"), allfiles)
    ordered_plyfiles = list(plyfiles)
    ordered_plyfiles.sort()

    # Work-around for an issue with OpenGL (used in cwipc_window): it will
    # change the working directory (!)
    cwd = os.getcwd()
    sink = cwipc.cwipc_window(sys.argv[0])
    os.chdir(cwd)

    ok = True
    while ok:
        for fn in ordered_plyfiles:
            fullpath = os.path.join(dirname, fn)
            fullpath = os.path.abspath(fullpath)
            if fullpath.endswith(".cwipcdump"):
                pc = cwipc.cwipc_read_debugdump(fullpath)
            else:
                pc = cwipc.cwipc_read(fullpath, 0)
            assert pc
            ok = sink.feed(pc, True)


if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
    