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

def _parse_aux_description(description : str) -> Dict[str, Any]:
    rv = {}
    fields = description.split(',')
    for f in fields:
        k, v = f.split('=')
        try:
            v = int(v)
        except ValueError:
            pass
        rv[k] = v
    return rv

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
    old_code = False
    if old_code:
        rgb_images = {}
        depth_images = {}
        print(f"auxdata has {auxdata.count()} items")
        for i in range(auxdata.count()):
            print(f"auxdata {i}: name={auxdata.name(i)}, description={auxdata.description(i)}")
            name = auxdata.name(i)
            if name.startswith("rgb."):
                serial = name[4:]
                descrstr = auxdata.description(i)
                descr = _parse_aux_description(descrstr)
                width = descr["width"]
                height = descr["height"]
                stride = descr["stride"]
                bpp = 0
                if "bpp" in descr:
                    bpp = descr["bpp"]
                else:
                    image_format = descr['format']
                    if image_format == 2:
                        bpp = 3 # RGB
                    elif image_format == 3:
                        bpp = 4 # RGBA
                    elif image_format == 4:
                        bpp = 2 # 16-bit grey
                assert bpp
                image_data = auxdata.data(i)
                np_image_data_bytes = np.array(image_data)
                np_image_data = np.reshape(np_image_data_bytes, (height, width, bpp))
                np_image_data = np_image_data[:,:,[0,1,2]]
                # Select B, G, R channels
                # np_image_data = np_image_data[:,:,[2,1,0]]
                rgb_images[serial] = np_image_data
            elif name.startswith("depth."):
                serial = name[6:]
                descrstr = auxdata.description(i)
                descr = _parse_aux_description(descrstr)
                width = descr["width"]
                height = descr["height"]
                stride = descr["stride"]
                bpp = 0
                if "bpp" in descr:
                    bpp = descr["bpp"]
                else:
                    image_format = descr['format']
                    if image_format == 2:
                        bpp = 3 # RGB
                    elif image_format == 3:
                        bpp = 4 # RGBA
                    elif image_format == 4:
                        bpp = 2 # 16-bit grey
                assert bpp
                image_data = auxdata.data(i)
                np_image_data_bytes = np.array(image_data)
                np_image_data = np.reshape(np_image_data_bytes, (height, width, bpp))
                depth_images[serial] = np_image_data
    else:
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
        if old_code:
            depth_bytes = depth_image[y, x]
            depth = float(depth_bytes[0] + depth_bytes[1]*256)
        else:
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
