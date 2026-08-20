import os
import ctypes
import ctypes.util
import warnings
from typing import Optional, Union
from cwipc.util import CwipcError, CWIPC_API_VERSION, cwipc_activesource_wrapper
from cwipc.util import cwipc_activesource_p
from cwipc.util import _cwipc_dll_search_path_collection

__all__ = [
    "RS2_FORMAT_RGB8",
    "RS2_FORMAT_Z16",
    "cwipc_get_version_module",
    "cwipc_realsense2",
    "cwipc_realsense2_playback",
    "cwipc_realsense2_dll_load"
]

cwipc_image_buffer_type = Union[bytearray, bytes, ctypes.Array[ctypes.c_char]]
#
# This is a workaround for the change in DLL loading semantics on Windows since Python 3.8
# Python no longer uses the PATH environment variable to load dependent dlls but only
# its own set. For that reason we list here a set of dependencies that we know are needed,
# find those on PATH, and add the directories where those DLLs are located while loading our
# DLL.
# The list does not have to be complete, as long as at least one DLL from each directory needed
# is listed.
# Dependencies of cwipc_util are automatically added.
# NOTE: this list must be kept up-to-date otherwise loading DLLs will fail with
# an obscure message "Python could not find module .... or one of its dependencies"
#
_WINDOWS_NEEDED_DLLS=[ # NOT USED AT THE TIME. CAUSING DLL Loading problems
    "realsense2",
]

_cwipc_realsense2_dll_reference = None

#
# NOTE: the signatures here must match those in cwipc_util/api.h or all hell will break loose
#
def cwipc_realsense2_dll_load(libname : Optional[str]=None):
    """Load the cwipc_realsense2 DLL and assign the signatures (if not already loaded).
    
    If you want to load a non-default native library (for example to allow debugging low level code)
    call this method early, before any other method from this package.
    """
    global _cwipc_realsense2_dll_reference
    if _cwipc_realsense2_dll_reference: return _cwipc_realsense2_dll_reference
    
    with _cwipc_dll_search_path_collection(None) as loader:
        if libname == None:
            libname = 'cwipc_realsense2'
        if not os.path.isabs(libname):
            libname = loader.find_library(libname)
            if not libname:
                raise RuntimeError('Dynamic library realsense2 not found')
        assert libname
        _cwipc_realsense2_dll_reference = ctypes.CDLL(libname)
        if not _cwipc_realsense2_dll_reference:
            raise RuntimeError(f'Dynamic library {libname} cannot be loaded')
    
    _cwipc_realsense2_dll_reference.cwipc_get_version_realsense2.argtypes = []
    _cwipc_realsense2_dll_reference.cwipc_get_version_realsense2.restype = ctypes.c_char_p
    _cwipc_realsense2_dll_reference.cwipc_realsense2.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_realsense2_dll_reference.cwipc_realsense2.restype = cwipc_activesource_p
    _cwipc_realsense2_dll_reference.cwipc_realsense2_playback.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_realsense2_dll_reference.cwipc_realsense2_playback.restype = cwipc_activesource_p

    return _cwipc_realsense2_dll_reference

def RS2_FORMAT_RGB8():
    return ctypes.c_int.in_dll(cwipc_realsense2_dll_load(), "CWIPC_RS2_FORMAT_RGB8")
    
def RS2_FORMAT_Z16():
    return ctypes.c_int.in_dll(cwipc_realsense2_dll_load(), "CWIPC_RS2_FORMAT_Z16")

def cwipc_get_version_module() -> str:
    c_version = cwipc_realsense2_dll_load().cwipc_get_version_realsense2()
    version = c_version.decode('utf8')
    return version

def cwipc_realsense2(conffile : Optional[str]=None) -> cwipc_activesource_wrapper:
    """Returns a cwipc_source object that grabs from a realsense2 camera and returns cwipc object on every get() call."""
    errorString = ctypes.c_char_p()
    cconffile = None
    if conffile:
        cconffile = conffile.encode('utf8')
    rv = cwipc_realsense2_dll_load().cwipc_realsense2(cconffile, ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not rv:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if rv:
        return cwipc_activesource_wrapper(rv)
    raise CwipcError("cwipc_realsense2: no cwipc_activesource created, but no specific error returned from C library")
        
def cwipc_realsense2_playback(conffile : str) -> cwipc_activesource_wrapper:
    """Returns a cwipc_source object that grabs from a realsense2 recording (.bag file) and returns cwipc object on every get() call."""
    errorString = ctypes.c_char_p()
    cconffile = None
    if conffile:
        cconffile = conffile.encode('utf8')
    rv = cwipc_realsense2_dll_load().cwipc_realsense2_playback(cconffile, ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not rv:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if rv:
        return cwipc_activesource_wrapper(rv)
    raise CwipcError("cwipc_realsense2_playback: no cwipc_activesource created, but no specific error returned from C library")