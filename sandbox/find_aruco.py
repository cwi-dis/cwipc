import sys
import cv2
import cv2.aruco

ARUCO_PARAMETERS = cv2.aruco.DetectorParameters()
ARUCO_DICT = cv2.aruco.getPredefinedDictionary(cv2.aruco.DICT_5X5_50)
ARUCO_DETECTOR = cv2.aruco.ArucoDetector(ARUCO_DICT, ARUCO_PARAMETERS)

def main():
    if len(sys.argv) <= 1:
        print("Usage: {sys.argv[0]} filename [...]", file=sys.stderr)
        sys.exit(1)
    for fn in sys.argv[1:]:
        if fn.endswith(".ply"):
            find_aruco_in_ply(fn)
        else:
            find_aruco_in_image(fn)

def find_aruco_in_ply(filename : str):
    assert False

def find_aruco_in_image(filename : str):
    if not cv2.haveImageReader(filename):
        assert False, f"No opencv image reader for {filename}"
    img = cv2.imread(filename)
    assert img is not None, "file could not be read, check with os.path.exists()"
    print('Shape', img.shape)
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

if __name__ == '__main__':
    main()