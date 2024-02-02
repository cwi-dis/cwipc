import sys
import cv2
from typing import Tuple, List
import numpy as np
import open3d as o3d
import cv2.typing
import cv2.aruco
import cwipc
import cwipc.registration.util

ARUCO_PARAMETERS = cv2.aruco.DetectorParameters()
ARUCO_DICT = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_5X5_50)
ARUCO_DETECTOR = cv2.aruco.ArucoDetector(ARUCO_DICT, ARUCO_PARAMETERS)

def main():
    if len(sys.argv) <= 1:
        print("Usage: {sys.argv[0]} filename [...]", file=sys.stderr)
        sys.exit(1)
    for fn in sys.argv[1:]:
        if fn.endswith(".ply"):
            find_aruco_in_plyfile(fn)
        else:
            find_aruco_in_imagefile(fn)

def find_aruco_in_plyfile(filename : str):
    full_pc = cwipc.cwipc_read(filename, 0)
    tiles_used = cwipc.registration.util.get_tiles_used(full_pc)
    for tile in tiles_used:
        pc = cwipc.cwipc_tilefilter(full_pc, tile)
        cwipc.registration.util.show_pointcloud(f"camera {tile}", pc, from000=True)
        find_aruco_in_pointcloud(pc)
    

def find_aruco_in_imagefile(filename : str):
    if not cv2.haveImageReader(filename):
        assert False, f"No opencv image reader for {filename}"
    img = cv2.imread(filename)
    assert img is not None, "file could not be read, check with os.path.exists()"
    print('Shape', img.shape)
    find_aruco_in_image(img)

def find_aruco_in_pointcloud(pc : cwipc.cwipc_wrapper):
    width = 1024
    height = 1024
    for rotation in [
                [0.0, 0.0, 0.0],
            ]:
        img_bgr, img_xyz = project_pointcloud_to_images(pc, width, height, rotation)
        find_aruco_in_image(img_bgr)


def find_aruco_in_image(img : cv2.typing.MatLike):
    corners, ids, rejected  = ARUCO_DETECTOR.detectMarkers(img)
    print("corners", corners)
    print("ids", ids)
    print("rejected", rejected)
    if True:
        outputImage = img.copy()
        cv2.aruco.drawDetectedMarkers(outputImage, corners, ids)
        cv2.imshow("Detected markers", outputImage)
        while True:
            ch = cv2.waitKey()
            if ch == 27:
                break
            print(f"ignoring key {ch}")
        cv2.destroyWindow("Detected markers")

def project_pointcloud_to_images(pc : cwipc.cwipc_wrapper, width : int, height : int, rotation: List[float]) -> Tuple[cv2.typing.MatLike, cv2.typing.MatLike]:

    img_bgr = np.zeros(shape=(width, height, 3), dtype=np.uint8)
    img_xyz = np.zeros(shape=(width, height, 3), dtype=np.float32)
    
    xyz_array_orig, rgb_array = _get_nparrays_for_pc(pc)
    xyz_pointcloud = o3d.geometry.PointCloud()
    xyz_pointcloud.points = o3d.utility.Vector3dVector(xyz_array_orig)
    # xxxjack should do transformation here
    angles = xyz_pointcloud.get_rotation_matrix_from_xyz(rotation)
    xyz_pointcloud.rotate(angles, center=[0, 0, 0])
    xyz_array = np.asarray(xyz_pointcloud.points)
    x_array = xyz_array[:,0]
    y_array = xyz_array[:,1]
    min_x = np.min(x_array)
    max_x = np.max(x_array)
    min_y = np.min(y_array)
    max_y = np.max(y_array)
    print(f"x range: {min_x}..{max_x}, y range: {min_y}..{max_y}")
    x_factor = (width-1) / (max_x - min_x)
    y_factor = (height-1) / (max_y - min_y)
    # I have _absolutely_ no idea why X has to be inverted....
    # Or is this yet another case of left-handed versus right-handed coordinate systems?
    invert_x = True
    invert_y = False
    # xxxjack should do this with numpy
    passes = (1,)
    for pass_ in passes:
        for i in range(len(xyz_array)):
            xyz = xyz_array[i]
            xyz_orig = xyz_array_orig[i] # Note: this is the original point, before any transformation 
            rgb = rgb_array[i]
            bgr = rgb[[2,1,0]]
            x = xyz[0]
            y = xyz[1]
            z = xyz[2]
            if z < 0:
                continue
            img_x = int((x-min_x) * x_factor)
            img_y = int((y-min_y) * y_factor)
            if invert_x:
                img_x = width-1-img_x
            if invert_y:
                img_y = height-1-img_y
            if pass_ == 0:
                for iix in range(max(0, img_x-2), min(width-1, img_x+2)):
                    for iiy in range(max(0, img_y-2), min(height-1, img_y+2)):
                            img_bgr[iiy][iix] = bgr
                            img_xyz[iiy][iix] = xyz_orig
            else:
                img_bgr[img_y][img_x] = bgr
                img_xyz[img_y][img_x] = xyz_orig
    return img_bgr, img_xyz

def _get_nparrays_for_pc(pc : cwipc.cwipc_wrapper) -> Tuple[cv2.typing.MatLike, cv2.typing.MatLike]:
    # Get the points (as a cwipc-style array) and convert them to a NumPy array-of-structs
    pointarray = np.ctypeslib.as_array(pc.get_points())
    # Extract the relevant fields (X, Y, Z coordinates)
    xyzarray = pointarray[['x', 'y', 'z']]
    rgbarray = pointarray[['r', 'g', 'b']]
    # Turn this into an N by 3 2-dimensional array
    np_xyzarray = np.column_stack([xyzarray['x'], xyzarray['y'], xyzarray['z']])
    np_rgbarray = np.column_stack([rgbarray['r'], rgbarray['g'], rgbarray['b']])
    return np_xyzarray, np_rgbarray

if __name__ == '__main__':
    main()