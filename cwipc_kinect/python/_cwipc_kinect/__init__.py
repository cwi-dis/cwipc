import os
import ctypes
import ctypes.util
import warnings
from typing import Optional
from cwipc.util import CwipcError, CWIPC_API_VERSION, cwipc_activesource_wrapper
from cwipc.util import cwipc_activesource_p
from cwipc.util import _cwipc_dll_search_path_collection # type: ignore

__all__ = [
    "cwipc_get_version_module",
    "cwipc_kinect",
    "cwipc_kinect_playback",
    "cwipc_kinect_dll_load"
]

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
    "k4a",
    "k4abt",
    "turbojpeg",
    "jpeg62"
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
    
    with _cwipc_dll_search_path_collection(None) as loader:
        if libname == None:
            libname = 'cwipc_kinect'
        if not os.path.isabs(libname):
            libname = loader.find_library(libname)
            if not libname:
                raise RuntimeError('Dynamic library cwipc_kinect not found')
        assert libname
        _cwipc_kinect_dll_reference = ctypes.CDLL(libname)
        if not _cwipc_kinect_dll_reference:
            raise RuntimeError(f'Dynamic library {libname} cannot be loaded')
    
    _cwipc_kinect_dll_reference.cwipc_get_version_kinect.argtypes = []
    _cwipc_kinect_dll_reference.cwipc_get_version_kinect.restype = ctypes.c_char_p

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