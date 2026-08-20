import sys
import os
import cwipc
import cwipc.codec

def main():
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} pointcloudfile.ply|.cwipcdump pointcloudfile.cwicpc", file=sys.stderr)
        sys.exit(2)
    
    ifn = sys.argv[1]
    ofn = sys.argv[2]
    if ifn.endswith(".ply"):
        pc = cwipc.cwipc_read(ifn, 0)
    else:
        pc = cwipc.cwipc_read_debugdump(ifn)

    # You can pass parameters to set things like octree_depth
    encoder = cwipc.codec.cwipc_new_encoder(params=dict(octree_bits=7))

    encoder.feed(pc)
    if not encoder.available(True):
        print("Encoder did not produce compressed point cloud?")
        sys.exit(1)
    data = encoder.get_bytes()

    with open(ofn, "wb") as fp:
        fp.write(data)

   
if __name__ == '__main__':
    main()
    cwipc.cwipc_dangling_allocations(True)
    