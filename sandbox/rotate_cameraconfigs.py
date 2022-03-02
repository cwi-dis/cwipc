import numpy as np
import glob
#code from cameracongig.xml
import copy
import xml.etree.ElementTree as ET

DEFAULT_CONFIGFILE="""<?xml version="1.0" ?>
<file>
    <CameraConfig version="2">
        <system  />
        <skeleton />
        <postprocessing height_min="0" height_max="0" radius_filter="0" greenscreenremoval="0">
            <depthfilterparameters  />
        </postprocessing>
        <camera serial="0" type="">
            <trafo>
                <values v00="1" v01="0" v02="0" v03="0" v10="0" v11="1" v12="0" v13="0" v20="0" v21="0" v22="1" v23="0" v30="0" v31="0" v32="0" v33="1"  />
            </trafo>
        </camera>
    </CameraConfig>
</file>
"""
FILTER_PARAMS_REALSENSE=dict(
    do_threshold="1",
    threshold_near="0.2",
    threshold_far="4",

    depth_x_erosion="2",
    depth_y_erosion="2",
    
    do_decimation="1",
    decimation_value="2",
    
    do_spatial="1",
    spatial_iterations="1",
    spatial_alpha="0.5",
    spatial_delta="20",
    spatial_filling="1",
    
    do_temporal="1",
    temporal_alpha="0.4",
    temporal_delta="20",
    temporal_percistency="3",
)

SYSTEM_PARAMS_REALSENSE=dict(
    usb2width="640",
    usb2height="480",
    usb2fps="15",
    usb3width="1280",
    usb3height="720",
    usb3fps="30",
    usb2allowed="0",
    exposure="-1",
    whitebalance="-1",
    backlight_compensation="0",
    laser_power="360",
    density_preferred="1"
)

SKELETON_PARAMS_REALSENSE=dict()

FILTER_PARAMS_KINECT=dict(
    do_threshold="1",
    threshold_near="0.2",
    threshold_far="4",

    depth_x_erosion="1",
    depth_y_erosion="1",
)

SYSTEM_PARAMS_KINECT=dict(
    color_height="720",
    depth_height="576",
    fps="15",
    sync_master_serial="",
    colormaster="1",
    color_exposure_time="-1",
    color_whitebalance="-1",
    color_backlight_compensation="-1",
    color_brightness="-1",
    color_contrast="-1",
    color_saturation="-1",
    color_sharpness="-1",
    color_gain="-1",
    color_powerline_frequency="-1"
)

SKELETON_PARAMS_KINECT=dict(
    sensor_orientation="-1",
    processing_mode="-1",
    model_path=""
)

DEFAULT_FILENAME="cameraconfig.xml"
DEFAULT_TYPE="realsense"
DEFAULT_FILTER_PARAMS=FILTER_PARAMS_REALSENSE
DEFAULT_SYSTEM_PARAMS=SYSTEM_PARAMS_REALSENSE
DEFAULT_SKELETON_PARAMS=SKELETON_PARAMS_REALSENSE

def selectCameraType(cameraType):
    global DEFAULT_TYPE, DEFAULT_FILTER_PARAMS, DEFAULT_SYSTEM_PARAMS, DEFAULT_SKELETON_PARAMS
    DEFAULT_TYPE = cameraType
    DEFAULT_FILTER_PARAMS = globals()[f'FILTER_PARAMS_{cameraType.upper()}']
    DEFAULT_SYSTEM_PARAMS = globals()[f'SYSTEM_PARAMS_{cameraType.upper()}']
    DEFAULT_SKELETON_PARAMS = globals()[f'SKELETON_PARAMS_{cameraType.upper()}']
    
class CameraConfig:

    def __init__(self, confFilename, read=True):
        self.confFilename = confFilename
        self.serials = []
        self.matrices = []
        self.tree = None        
        if read:
            self._readConf(self.confFilename)
            self._parseConf()
        
    def _readConf(self, confFilename):
        self.tree = ET.parse(confFilename)
        
    def copyFrom(self, other):
        self.tree = copy.deepcopy(other.tree)
        self._parseConf()
        
    def fillDefault(self):
        root = ET.fromstring(DEFAULT_CONFIGFILE)
        paramElt = root.find('CameraConfig/postprocessing/depthfilterparameters')
        for k, v in DEFAULT_FILTER_PARAMS.items():
            paramElt.set(k, v)
        paramElt = root.find('CameraConfig/system')
        for k, v in DEFAULT_SYSTEM_PARAMS.items():
            paramElt.set(k, v)
        paramElt = root.find('CameraConfig/skeleton')
        for k, v in DEFAULT_SKELETON_PARAMS.items():
            paramElt.set(k, v)
        self.tree = ET.ElementTree(root)
        
        self._parseConf()
        
    def _parseConf(self):
        root = self.tree.getroot()
        for camElt in root.findall('CameraConfig/camera'):
            serial = camElt.attrib['serial']
            assert serial
            trafoElt = camElt.find('trafo')
            valuesElt = trafoElt.find('values')
            va = valuesElt.attrib
            trafo = [
                [float(va['v00']), float(va['v01']), float(va['v02']), float(va['v03'])],
                [float(va['v10']), float(va['v11']), float(va['v12']), float(va['v13'])],
                [float(va['v20']), float(va['v21']), float(va['v22']), float(va['v23'])],
                [float(va['v30']), float(va['v31']), float(va['v32']), float(va['v33'])],
            ]
            self.serials.append(serial)
            self.matrices.append(trafo)
        
    def save(self):
        self.tree.write(self.confFilename)
        
    def savef(self, filename):
        self.tree.write(filename)
        
    def getcount(self):
        return len(self.serials)
        
    def getserials(self):
        return self.serials
        
    def getmatrix(self, tilenum):
        return self.matrices[tilenum]
        
    def addcamera(self, serial):
        root = self.tree.getroot()
        firstCamElt = root.find('CameraConfig/camera')
        newCamElt = copy.deepcopy(firstCamElt)
        newCamElt.set('serial', serial)
        newCamElt.set('type', DEFAULT_TYPE)
        ccElt = root.find('CameraConfig')
        ccElt.append(newCamElt)
        
        self.serials.append(serial)
        matrix = [[1, 0, 0, 0], [0, 1, 0, 0], [0, 0, 1, 0], [0, 0, 0, 1]]
        self.matrices.append(matrix)
        
        self.setmatrix(len(self.serials)-1, matrix)
        
    def setmatrix(self, tilenum, matrix):
        self.matrices[tilenum] = copy.deepcopy(matrix)
        serial = self.serials[tilenum]
        root = self.tree.getroot()
        camElt = root.find(f"CameraConfig/camera[@serial='{serial}']")
        trafoElt = camElt.find('trafo')
        valuesElt = trafoElt.find('values')
        valuesElt.set('v00', str(matrix[0][0]))
        valuesElt.set('v01', str(matrix[0][1]))
        valuesElt.set('v02', str(matrix[0][2]))
        valuesElt.set('v03', str(matrix[0][3]))
        valuesElt.set('v10', str(matrix[1][0]))
        valuesElt.set('v11', str(matrix[1][1]))
        valuesElt.set('v12', str(matrix[1][2]))
        valuesElt.set('v13', str(matrix[1][3]))
        valuesElt.set('v20', str(matrix[2][0]))
        valuesElt.set('v21', str(matrix[2][1]))
        valuesElt.set('v22', str(matrix[2][2]))
        valuesElt.set('v23', str(matrix[2][3]))
        valuesElt.set('v30', str(matrix[3][0]))
        valuesElt.set('v31', str(matrix[3][1]))
        valuesElt.set('v32', str(matrix[3][2]))
        valuesElt.set('v33', str(matrix[3][3]))
        
    def setserial(self, tilenum, serial):
        oldSerial = self.serials[tilenum]
        self.serials[tilenum] = serial
        root = self.tree.getroot()
        camElt = root.find(f"CameraConfig/camera[@serial='{oldSerial}']")
        camElt.set('serial', serial)
        camElt.set('type', DEFAULT_TYPE)
        
    def setdistance(self, threshold_near, threshold_far):
        root = self.tree.getroot()
        dfElt = root.find('CameraConfig/postprocessing/depthfilterparameters')
        dfElt.set('do_threshold', "1")
        dfElt.set('threshold_near', str(threshold_near))
        dfElt.set('threshold_far', str(threshold_far))
        
    def setheight(self, height_min, height_max):
        root = self.tree.getroot()
        ppElt = root.find('CameraConfig/postprocessing')
        ppElt.set('height_min', str(height_min))
        ppElt.set('height_max', str(height_max))
        
    def setsystemparam(self, name, value):
        root = self.tree.getroot()
        sysElt = root.find('CameraConfig/system')
        value = str(value)
        sysElt.set(name, value)
        
    def setfilterparam(self, name, value):
        root = self.tree.getroot()
        dfElt = root.find('CameraConfig/postprocessing/depthfilterparameters')
        value = str(value)
        dfElt.set(name, value)
#endcode cameraconfig.xml

filenames = glob.glob('cwipc-sxr-dataset/**/*.xml', recursive=True)
for filename in filenames:
    cameraconfig = CameraConfig(filename, read=True)

    #create copy
    cameraconfig.savef(filename+'_old')

    #we rotate 180degrees on y axis
    r_angle = np.radians(180)
    ry_matrix = [[np.cos(r_angle), 0, np.sin(r_angle), 0],
                [0, 1, 0, 0],
                [-np.sin(r_angle), 0, np.cos(r_angle), 0],
                [0, 0, 0, 1]]
    m = 0
    for matrix in cameraconfig.matrices:
        nmatrix = np.asarray(ry_matrix) @ np.asarray(matrix) #nice discovery: if we invert the multiplication order, the result is the point of view of each of the cameras
        cameraconfig.setmatrix(m, nmatrix)
        m+=1

    #save modified cameraconfig
    cameraconfig.save()