import sys
import struct
from typing import Dict, Any
import numpy as np
import cwipc
try:
    import cwipc.realsense2
    _ = cwipc.realsense2.cwipc_realsense2_dll_load()
except:
    pass
try:
    import cwipc.kinect
    _ = cwipc.kinect.cwipc_kinect_dll_load()
except:
    pass

import os
print(f"PID={os.getpid()}")

# Check that we can convert image (x, y) positions to 3D (x, y, z) positions
def main():
    configfile = sys.argv[1]
    x = int(sys.argv[2])
    y = int(sys.argv[3])
    grabber = cwipc.cwipc_capturer(configfile)
    grabber.request_auxiliary_data("rgb")
    grabber.request_auxiliary_data("depth")
    while True:
        while not grabber.available(True):
            print("Waiting for grab")
        pc = grabber.get()
        if not pc:
            print("Skip empty pc")
            continue
        auxdata = pc.access_auxiliary_data()
        assert auxdata
        if auxdata.count() > 0:
            break
    
    rgb_images = auxdata.get_all_images("rgb.")
    depth_images = auxdata.get_all_images("depth.")
    serials = rgb_images.keys()
    serial_to_tilenum = {}
    for t in range(grabber.maxtile()):
        tileinfo = grabber.get_tileinfo_dict(t)
        serial = tileinfo["cameraName"]
        if serial:
            serial = serial.decode('utf8')
            print(f"serial {serial}: tileMask {tileinfo['cameraMask']}")
            serial_to_tilenum[serial] = tileinfo["cameraMask"]
    for serial in serials:
        rgb_image = rgb_images[serial]
        depth_image = depth_images[serial]
        rgb = rgb_image[y, x]
        depth = float(depth_image[y, x])
        print(f"serial {serial}: point={(x, y)}, distance={depth}, color={rgb}")
        # Pack the arguments
        arg_tilenum = float(serial_to_tilenum[serial])
        arg_x = float(x)
        arg_y = float(y)
        arg_depth = float(depth)
        inargs = struct.pack("ffff", arg_tilenum, arg_x, arg_y, arg_depth)
        outargs = bytearray(12)
        ok = grabber.auxiliary_operation("map2d3d", inargs, outargs)
        if not ok:
            print(f"serial {serial}: map2d3d failed")
            continue
        rv_x, rv_y, rv_z = struct.unpack("fff", outargs)
        print(f"serial {serial}: x={rv_x}, y={rv_y}, z={rv_z}")



if __name__ == '__main__':
    main()
