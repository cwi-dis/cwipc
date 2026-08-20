import os
import json
from typing import Optional, List, cast, Tuple, Dict
import numpy
import cwipc
from cwipc.net.abstract import *
from .abstract import *
from .util import *

PythonTrafo = List[List[float]]

class Transform:
    """A 4x4 spatial transformation matrix, represented as both a list of lists of floats or a numpy matrix."""
    trafo : PythonTrafo
    matrix : RegistrationTransformation
    changed : bool

    def __init__(self, trafo : PythonTrafo):
        self._set(trafo)
        self.changed = False

    def is_dirty(self) -> bool:
        return self.changed
    
    def reset(self) -> None:
        self.changed = False

    def get(self) -> PythonTrafo:
        return self.trafo
    
    def _set(self, trafo : PythonTrafo) -> None:
        self.trafo = trafo
        self.matrix = transformation_frompython(self.trafo)

    def get_matrix(self) -> RegistrationTransformation:
        return self.matrix
    
    def set_matrix(self, matrix : RegistrationTransformation) -> None:
        if numpy.array_equal(matrix, self.matrix):
            return
        self.matrix = matrix
        self.trafo = transformation_topython(self.matrix)
        self.changed = True

    def apply_matrix(self, matrix : RegistrationTransformation) -> None:
        old_transform = self.get_matrix()
        new_transform = np.matmul(matrix, old_transform)
        self.set_matrix(new_transform)

    def is_identity(self) -> bool:
        return numpy.array_equal(self.matrix, transformation_identity())
    
class CameraConfig:
    transforms : List[Transform]

    def __init__(self, filename):
        self.filename = filename
        self.cameraconfig = dict()
        oldconfig = self.filename + '~'
        if os.path.exists(oldconfig):
            os.unlink(oldconfig)
        self.transforms = []
        self._dirty = False

    def init_transforms(self) -> None:
        cameras = self["camera"]
        camera_count = len(cameras)
        self.transforms = [
            Transform(cam["trafo"]) for cam in cameras
        ]

    def camera_count(self) -> int:
        return len(self.transforms)
    
    def is_dirty(self) -> bool:
        if self._dirty:
            return True
        for t in self.transforms:
            if t.is_dirty():
                return True
        return False
    
    def get_transform(self, camnum : int) -> Transform:
        return self.transforms[camnum]
    
    def refresh_transforms(self) -> None:
        """Copy transforms back to cameraconfig"""
        for cam_num in range(len(self.transforms)):
            trafo = self.transforms[cam_num].get()
            self.cameraconfig["camera"][cam_num]["trafo"] = trafo
    
    def is_identity(self) -> bool:
        for t in self.transforms:
            if not t.is_identity():
                return False
        return True

    def get_serial_dict(self) -> Dict[int, str]:
        """Return a mapping from tilemask to camera serial number"""
        rv = {}
        if len(self.cameraconfig) == 1:
            mask = 0
        else:
            mask = 1
        for cam in self.cameraconfig["camera"]:
            serial = cam["serial"]
            rv[mask] = serial
            mask <<= 1
        return rv

    def load(self, jsondata : bytes) -> None:
        self.cameraconfig = json.loads(jsondata)
        self.init_transforms()
        self._dirty = False
        # Workaround for bug (only in realsense_playback?) 2023-12-30
        self.cameraconfig["type"] = self.cameraconfig["camera"][0]["type"]

    def load_from_file(self) -> None:
        jsondata = open(self.filename, 'rb').read()
        self.load(jsondata)
        
    def save(self) -> None:
        # First time keep the original config file
        self.refresh_transforms()
        oldconfig = self.filename + '~'
        if os.path.exists(self.filename) and not os.path.exists(oldconfig):
            os.rename(self.filename, oldconfig)
        json.dump(self.cameraconfig, open(self.filename, 'w'), indent=2)
        # Mark transforms as not being dirty any longer.
        for t in self.transforms:
            t.reset()
        self._dirty = False
        print(f"cwipc_register: saved {self.filename}")

    def save_to(self, filename : str) -> None:
        self.refresh_transforms()
        json.dump(self.cameraconfig, open(filename, 'w'), indent=2)

    def get(self) -> bytes:
        return json.dumps(self.cameraconfig).encode('utf8')
    
    def __setitem__(self, key, item):
        self.cameraconfig[key] = item
        self._dirty = True

    def __getitem__(self, key):
        return self.cameraconfig[key]
    
    def set_entry_from_string(self, arg : str) -> bool:
        key, value = arg.split('=')
        value = eval(value)
        key_list = key.split(".")
        d = self.cameraconfig
        for k in key_list[:-1]:
            d = d[k]
        k = key_list[-1]
        if d.get(k) != value:
            d[k] = value
            self._dirty = 1
            return True
        return False
