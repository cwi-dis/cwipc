import os
import ctypes
import ctypes.util
import warnings
from typing import Optional
from cwipc.util import CwipcError, CWIPC_API_VERSION, cwipc_activesource_wrapper
from cwipc.util import cwipc_activesource_p
from cwipc.util import _cwipc_load_library # type: ignore

__all__ = [
    "cwipc_get_version_module",
    "cwipc_kinect",
    "cwipc_kinect_playback",
    "cwipc_kinect_dll_load"
]


_cwipc_kinect_dll_reference = None

#
# NOTE: the signatures here must match those in cwipc_util/api.h or all hell will break loose
#
def cwipc_kinect_dll_load(libname : Optional[str]=None) -> ctypes.CDLL:
    """Load the cwipc_kinect DLL and assign the signatures (if not already loaded).
    
    If you want to load a non-default native library (for example to allow debugging low level code)
    call this method early, before any other method from this package.
    """
    global _cwipc_kinect_dll_reference
    if _cwipc_kinect_dll_reference: return _cwipc_kinect_dll_reference
    
    _cwipc_kinect_dll_reference = _cwipc_load_library('cwipc_kinect', libname, 'cwipc_kinect', 'cwipc_get_version_kinect', __file__)

    _cwipc_kinect_dll_reference.cwipc_kinect.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_kinect_dll_reference.cwipc_kinect.restype = cwipc_activesource_p
    
    _cwipc_kinect_dll_reference.cwipc_kinect_playback.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_kinect_dll_reference.cwipc_kinect_playback.restype = cwipc_activesource_p

    return _cwipc_kinect_dll_reference
        

def cwipc_get_version_module() -> str:
    c_version = cwipc_kinect_dll_load().cwipc_get_version_kinect()
    version = c_version.decode('utf8')
    return version

def cwipc_kinect(conffile : Optional[str]=None) -> cwipc_activesource_wrapper:
    """Returns a cwipc_source object that grabs from a kinect camera and returns cwipc object on every get() call."""
    errorString = ctypes.c_char_p()
    cconffile = None
    if conffile:
        cconffile = conffile.encode('utf8')
    rv = cwipc_kinect_dll_load().cwipc_kinect(cconffile, ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not rv:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if rv:
        return cwipc_activesource_wrapper(rv)
    raise CwipcError("cwipc_kinect: no cwipc_activesource created, but no specific error returned from C library")

def cwipc_kinect_playback(conffile : Optional[str]=None) -> cwipc_activesource_wrapper:
    """Returns a cwipc_source object that grabs from kinect camera recordings and returns cwipc objects on every get() call."""
    errorString = ctypes.c_char_p()
    cconffile = None
    if conffile:
        cconffile = conffile.encode('utf8')
    rv = cwipc_kinect_dll_load().cwipc_kinect_playback(cconffile, ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not rv:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if rv:
        return cwipc_activesource_wrapper(rv)
    raise CwipcError("cwipc_kinect_playback: no cwipc_activesource created, but no specific error returned from C library")