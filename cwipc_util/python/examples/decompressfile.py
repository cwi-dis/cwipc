import sys
import os
import cwipc
import cwipc.codec

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} pointcloudfile.cwicpc pointcloudfile.ply|.cwipcdump", file=sys.stderr)
        sys.exit(2)
    
    ifn = sys.argv[1]
    ofn = sys.argv[2]

    with open(ifn, "rb") as fp:
        data = fp.read()

    decoder = cwipc.codec.cwipc_new_decoder()

    decoder.feed(data)
    if not decoder.available(True):
        print("Decoder did not produce point cloud?")
        sys.exit(1)

    pc = decoder.get()
    assert pc

    if ofn.endswith(".ply"):
        cwipc.cwipc_write(ofn, pc)
    else:
        cwipc.cwipc_write_debugdump(ofn, pc)

   
if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
    