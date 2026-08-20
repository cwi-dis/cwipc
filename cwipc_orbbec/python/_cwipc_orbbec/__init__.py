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
    "cwipc_orbbec",
    "cwipc_orbbec_playback",
    "cwipc_orbbec_dll_load"
]

_cwipc_orbbec_dll_reference = None

#
# NOTE: the signatures here must match those in cwipc_util/api.h or all hell will break loose
#
def cwipc_orbbec_dll_load(libname : Optional[str]=None) -> ctypes.CDLL:
    """Load the cwipc_orbbec DLL and assign the signatures (if not already loaded).
    
    If you want to load a non-default native library (for example to allow debugging low level code)
    call this method early, before any other method from this package.
    """
    global _cwipc_orbbec_dll_reference
    if _cwipc_orbbec_dll_reference: return _cwipc_orbbec_dll_reference
    
    with _cwipc_dll_search_path_collection(None) as loader:
        if libname == None:
            libname = 'cwipc_orbbec'
        if not os.path.isabs(libname):
            libname = loader.find_library(libname)
            if not libname:
                raise RuntimeError('Dynamic library cwipc_orbbec not found')
        assert libname
        _cwipc_orbbec_dll_reference = ctypes.CDLL(libname)
        if not _cwipc_orbbec_dll_reference:
            raise RuntimeError(f'Dynamic library {libname} cannot be loaded')
    
    _cwipc_orbbec_dll_reference.cwipc_get_version_orbbec.argtypes = []
    _cwipc_orbbec_dll_reference.cwipc_get_version_orbbec.restype = ctypes.c_char_p

    _cwipc_orbbec_dll_reference.cwipc_orbbec.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_orbbec_dll_reference.cwipc_orbbec.restype = cwipc_activesource_p
    
    _cwipc_orbbec_dll_reference.cwipc_orbbec_playback.argtypes = [ctypes.c_char_p, ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_orbbec_dll_reference.cwipc_orbbec_playback.restype = cwipc_activesource_p
    return _cwipc_orbbec_dll_reference


def cwipc_get_version_module() -> str:
    c_version = cwipc_orbbec_dll_load().cwipc_get_version_orbbec()
    version = c_version.decode('utf8')
    return version
 
def cwipc_orbbec(conffile : Optional[str]=None) -> cwipc_activesource_wrapper:
    """Returns a cwipc_source object that grabs from a orbbec camera and returns cwipc object on every get() call."""
    errorString = ctypes.c_char_p()
    cconffile = None
    if conffile:
        cconffile = conffile.encode('utf8')
    rv = cwipc_orbbec_dll_load().cwipc_orbbec(cconffile, ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not rv:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if rv:
        return cwipc_activesource_wrapper(rv)
    raise CwipcError("cwipc_orbbec: no cwipc_activesource created, but no specific error returned from C library")

def cwipc_orbbec_playback(conffile : Optional[str]=None) -> cwipc_activesource_wrapper:
    """Returns a cwipc_source object that grabs from orbbec camera recordings and returns cwipc objects on every get() call."""
    errorString = ctypes.c_char_p()
    cconffile = None
    if conffile:
        cconffile = conffile.encode('utf8')
    rv = cwipc_orbbec_dll_load().cwipc_orbbec_playback(cconffile, ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not rv:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if rv:
        return cwipc_activesource_wrapper(rv)
    raise CwipcError("cwipc_orbbecplayback: no cwipc_activesource created, but no specific error returned from C library")