import os
import ctypes
import ctypes.util
import warnings
from typing import Optional, Any,Union
from cwipc.util import CwipcError, CWIPC_API_VERSION, cwipc_pointcloud_wrapper, cwipc_source_wrapper
from cwipc.util import cwipc_pointcloud_p, cwipc_source_p
from cwipc.util import _cwipc_dll_search_path_collection # type: ignore
from cwipc.util import cwipc_log_default_callback, CWIPC_LOG_LEVEL_WARNING

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
    "turbojpeg",
    "jpeg62"
]

class cwipc_encoder_p(ctypes.c_void_p):
    pass
    
class cwipc_encodergroup_p(ctypes.c_void_p):
    pass
    
class cwipc_decoder_p(cwipc_source_p):
    pass
    
_cwipc_codec_dll_reference = None

#
# NOTE: the signatures here must match those in cwipc_util/api.h or all hell will break loose
#
def cwipc_codec_dll_load(libname : Optional[str]=None) -> ctypes.CDLL:
    """Load the cwipc_codec DLL and assign the signatures (if not already loaded).
    
    If you want to load a non-default native library (for example to allow debugging low level code)
    call this method early, before any other method from this package.
    """
    global _cwipc_codec_dll_reference
    if _cwipc_codec_dll_reference: return _cwipc_codec_dll_reference
    
    with _cwipc_dll_search_path_collection(None) as loader:
        if libname == None:
            libname = 'cwipc_codec'
        if not os.path.isabs(libname):
            libname = loader.find_library(libname)
            if not libname:
                raise RuntimeError('Dynamic library cwipc_codec not found')
        assert libname
        _cwipc_codec_dll_reference = ctypes.CDLL(libname)
        if not _cwipc_codec_dll_reference:
            raise RuntimeError(f'Dynamic library {libname} cannot be loaded')
    
    _cwipc_codec_dll_reference.cwipc_get_version_codec.argtypes = []
    _cwipc_codec_dll_reference.cwipc_get_version_codec.restype = ctypes.c_char_p

    _cwipc_codec_dll_reference.cwipc_new_encoder.argtypes = [ctypes.c_int, ctypes.POINTER(cwipc_encoder_params), ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_codec_dll_reference.cwipc_new_encoder.restype = cwipc_encoder_p
    _cwipc_codec_dll_reference.cwipc_encoder_free.argtypes = [cwipc_encoder_p]
    _cwipc_codec_dll_reference.cwipc_encoder_free.restype = None
    _cwipc_codec_dll_reference.cwipc_encoder_available.argtypes = [cwipc_encoder_p, ctypes.c_bool]
    _cwipc_codec_dll_reference.cwipc_encoder_available.restype = ctypes.c_bool
    _cwipc_codec_dll_reference.cwipc_encoder_eof.argtypes = [cwipc_encoder_p]
    _cwipc_codec_dll_reference.cwipc_encoder_eof.restype = ctypes.c_bool
    _cwipc_codec_dll_reference.cwipc_encoder_at_gop_boundary.argtypes = [cwipc_encoder_p]
    _cwipc_codec_dll_reference.cwipc_encoder_at_gop_boundary.restype = ctypes.c_bool
    _cwipc_codec_dll_reference.cwipc_encoder_feed.argtypes = [cwipc_encoder_p, cwipc_pointcloud_p]
    _cwipc_codec_dll_reference.cwipc_encoder_feed.restype = None
    _cwipc_codec_dll_reference.cwipc_encoder_get_encoded_size.argtypes = [cwipc_encoder_p]
    _cwipc_codec_dll_reference.cwipc_encoder_get_encoded_size.restype = ctypes.c_size_t
    _cwipc_codec_dll_reference.cwipc_encoder_copy_data.argtypes = [cwipc_encoder_p, ctypes.c_void_p, ctypes.c_size_t]
    _cwipc_codec_dll_reference.cwipc_encoder_copy_data.restype = ctypes.c_bool
    _cwipc_codec_dll_reference.cwipc_encoder_close.argtypes = [cwipc_encoder_p]
    _cwipc_codec_dll_reference.cwipc_encoder_close.restype = None

    _cwipc_codec_dll_reference.cwipc_new_encodergroup.argtypes = [ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_codec_dll_reference.cwipc_new_encodergroup.restype = cwipc_encodergroup_p
    _cwipc_codec_dll_reference.cwipc_encodergroup_addencoder.argtypes = [cwipc_encodergroup_p, ctypes.c_int, ctypes.POINTER(cwipc_encoder_params), ctypes.POINTER(ctypes.c_char_p)]
    _cwipc_codec_dll_reference.cwipc_encodergroup_addencoder.restype = cwipc_encoder_p
    _cwipc_codec_dll_reference.cwipc_encodergroup_feed.argtypes = [cwipc_encodergroup_p, cwipc_pointcloud_p]
    _cwipc_codec_dll_reference.cwipc_encodergroup_feed.restype = None
    _cwipc_codec_dll_reference.cwipc_encodergroup_close.argtypes = [cwipc_encodergroup_p]
    _cwipc_codec_dll_reference.cwipc_encodergroup_close.restype = None

    _cwipc_codec_dll_reference.cwipc_new_decoder.argtypes = [ctypes.POINTER(ctypes.c_char_p), ctypes.c_ulong]
    _cwipc_codec_dll_reference.cwipc_new_decoder.restype = cwipc_decoder_p
    _cwipc_codec_dll_reference.cwipc_decoder_feed.argtypes = [cwipc_decoder_p, ctypes.c_void_p, ctypes.c_size_t]
    _cwipc_codec_dll_reference.cwipc_decoder_feed.restype = None
    _cwipc_codec_dll_reference.cwipc_decoder_close.argtypes = [cwipc_decoder_p]
    _cwipc_codec_dll_reference.cwipc_decoder_close.restype = None

    return _cwipc_codec_dll_reference

class cwipc_encoder_params(ctypes.Structure):
    """Parameters to control cwipc_encoder compression"""
    _fields_ = [
        ("do_inter_frame", ctypes.c_bool),
        ("gop_size", ctypes.c_int),
        ("exp_factor", ctypes.c_float),
        ("octree_bits", ctypes.c_int),
        ("jpeg_quality", ctypes.c_int),
        ("macroblock_size", ctypes.c_int),
        ("tilenumber", ctypes.c_int),
        ("voxelsize", ctypes.c_float),
        ("n_parallel", ctypes.c_int),
        ]
    
CWIPC_ENCODER_PARAM_VERSION = 0x20220607

class cwipc_encoder_wrapper:
    _cwipc_encoder : Optional[cwipc_encoder_p]
    _must_be_freed : bool

    def __init__(self, _cwipc_encoder : Optional[cwipc_encoder_p]):
        if _cwipc_encoder != None:
            assert isinstance(_cwipc_encoder, cwipc_encoder_p)
        self._cwipc_encoder = _cwipc_encoder
        self._must_be_freed = True

    def __del__(self):
        if self._must_be_freed:
            self.free(force=True)

    def _as_cwipc_encoder_p(self) -> cwipc_encoder_p:
        assert self._cwipc_encoder
        return self._cwipc_encoder
        
    def free(self, force:bool = False) -> None:
        if self._cwipc_encoder and self._must_be_freed:
            if not force:
                cwipc_log_default_callback(CWIPC_LOG_LEVEL_WARNING, b'cwipc_encoder_wrapper.free() called explicitly.')
            cwipc_codec_dll_load().cwipc_encoder_free(self._as_cwipc_encoder_p())
        self._cwipc_encoder = None
        self._must_be_freed = False

    def detach(self) -> 'cwipc_encoder_wrapper':
        """Detach the underlying cwipc_pointcloud_p pointer from this wrapper.
        
        After the call, this pointcloud is invalidated. The return value has the pointer
        to the underlying data that used to be on this object, but it will _not_ be freed
        when the Python object is deleted. The intention is that the returned object is
        passed to another language that will take ownership of it.
        """
        if self._cwipc == None:
            cwipc_log_default_callback(CWIPC_LOG_LEVEL_WARNING, b"detach() called on NULL pointer")
        rv = type(self)(self._cwipc)
        rv._must_be_freed = False
        self._cwipc = None
        self._must_be_freed = False
        return rv
    
    def close(self) -> None:
        if self._cwipc_encoder:
            cwipc_codec_dll_load().cwipc_encoder_close(self._as_cwipc_encoder_p())
        
    def eof(self) -> bool:
        rv = cwipc_codec_dll_load().cwipc_encoder_eof(self._as_cwipc_encoder_p())
        return rv
        
    def at_gop_boundary(self) -> bool:
        rv = cwipc_codec_dll_load().cwipc_encoder_at_gop_boundary(self._as_cwipc_encoder_p())
        return rv
        
    def available(self, wait : bool) -> bool:
        rv = cwipc_codec_dll_load().cwipc_encoder_available(self._as_cwipc_encoder_p(), wait)
        return rv
        
    def feed(self, pc: cwipc_pointcloud_wrapper) -> None:
        """Feed the point cloud to the encoder.
        """
        if pc == None:
            cpc = None
        else:
            cpc = pc.as_cwipc_p()
        rv = cwipc_codec_dll_load().cwipc_encoder_feed(self._as_cwipc_encoder_p(), cpc)
        return rv
        
    def get_encoded_size(self) -> int:
        rv = cwipc_codec_dll_load().cwipc_encoder_get_encoded_size(self._as_cwipc_encoder_p())
        return rv
        
    def get_bytes(self) -> bytearray:
        length = self.get_encoded_size()
        rv = bytearray(length)
        ptr_char = (ctypes.c_char * length).from_buffer(rv)
        ptr = ctypes.cast(ptr_char, ctypes.c_void_p)
        ok = cwipc_codec_dll_load().cwipc_encoder_copy_data(self._as_cwipc_encoder_p(), ptr, length)
        if ok:
            return rv
        raise CwipcError("get_bytes: cwipc_encoder_copy_data failed to return any data")
        
class cwipc_encodergroup_wrapper:
    _cwipc_encodergroup : Optional[cwipc_encodergroup_p]
    _must_be_freed : bool

    def __init__(self, _cwipc_encodergroup : Optional[cwipc_encodergroup_p]):
        if _cwipc_encodergroup != None:
            if not isinstance(_cwipc_encodergroup, cwipc_encodergroup_p):
                raise CwipcError("Invalid cwipc_encodergroup_p passed to cwipc_encodergroup_wrapper")
        self._cwipc_encodergroup = _cwipc_encodergroup
        self._must_be_freed = True
        
    def __del__(self):
        if self._must_be_freed:
            self.free(force=True)

    def _as_cwipc_encodergroup_p(self) -> cwipc_encodergroup_p:
        assert self._cwipc_encodergroup
        return self._cwipc_encodergroup
        
    def free(self, *, force : bool = False) -> None:
        if self._cwipc_encodergroup and self._must_be_freed:
            if not force:
                cwipc_log_default_callback(CWIPC_LOG_LEVEL_WARNING, b'cwipc_encodergroup_wrapper.free() called explicitly.')
            cwipc_codec_dll_load().cwipc_encodergroup_free(self._as_cwipc_encodergroup_p())
        self._cwipc_encodergroup = None
        self._must_be_freed = False

    def detach(self) -> 'cwipc_encodergroup_wrapper':
        """Detach the underlying cwipc_pointcloud_p pointer from this wrapper.
        
        After the call, this pointcloud is invalidated. The return value has the pointer
        to the underlying data that used to be on this object, but it will _not_ be freed
        when the Python object is deleted. The intention is that the returned object is
        passed to another language that will take ownership of it.
        """
        if self._cwipc == None:
            cwipc_log_default_callback(CWIPC_LOG_LEVEL_WARNING, b"detach() called on NULL pointer")
        rv = type(self)(self._cwipc)
        rv._must_be_freed = False
        self._cwipc = None
        self._must_be_freed = False
        return rv
    
    def close(self) -> None:
        if self._cwipc_encodergroup:
            cwipc_codec_dll_load().cwipc_encodergroup_close(self._as_cwipc_encodergroup_p())

    def feed(self, pc : cwipc_pointcloud_wrapper) -> None:
        if pc == None:
            cpc = None
        else:
            cpc = pc.as_cwipc_p()
        rv = cwipc_codec_dll_load().cwipc_encodergroup_feed(self._as_cwipc_encodergroup_p(), cpc)
        return rv
        
    def addencoder(self, version : Optional[int]=None, params : Union[dict[str,Any],cwipc_encoder_params, None]=None, **kwargs : Any) -> cwipc_encoder_wrapper:
        if version == None:
            version = CWIPC_ENCODER_PARAM_VERSION
        if isinstance(params, cwipc_encoder_params):
            pass
        else:
            params = cwipc_new_encoder_params(**kwargs)
        errorString = ctypes.c_char_p()
        obj = cwipc_codec_dll_load().cwipc_encodergroup_addencoder(self._as_cwipc_encodergroup_p(), version, params, ctypes.byref(errorString))
        if errorString and errorString.value and not obj:
            raise CwipcError(errorString.value.decode('utf8'))
        if errorString and errorString.value:
            warnings.warn(errorString.value.decode('utf8'))
        if obj:
            return cwipc_encoder_wrapper(obj)
        raise CwipcError("addencoder: failed, but no specific error returned from C library")

class cwipc_decoder_wrapper(cwipc_source_wrapper):
    def __init__(self, _cwipc_decoder : Optional[cwipc_decoder_p]):
        if _cwipc_decoder != None:
            assert isinstance(_cwipc_decoder, cwipc_decoder_p)
        cwipc_source_wrapper.__init__(self, _cwipc_decoder)
        
    def _as_cwipc_decoder_p(self) -> cwipc_source_p:
        assert self._cwipc_source
        return self._cwipc_source
        
    def feed(self, buffer : Union[bytes, bytearray, ctypes.Array[ctypes.c_char]]) -> None:
        length = len(buffer)
        if isinstance(buffer, bytearray):
            buffer = (ctypes.c_char * length).from_buffer(buffer)
        elif isinstance(buffer, bytes):
            buffer = (ctypes.c_char * length).from_buffer_copy(buffer)
        ptr = ctypes.cast(buffer, ctypes.c_void_p)
        rv = cwipc_codec_dll_load().cwipc_decoder_feed(self._as_cwipc_decoder_p(), ptr, length)
        return rv

    def close(self) -> None:
        if self._cwipc_source:
            cwipc_codec_dll_load().cwipc_decoder_close(self._as_cwipc_decoder_p())

def cwipc_new_encoder_params(**kwargs : Any) -> cwipc_encoder_params:
    params = cwipc_encoder_params(False, 1, 1, 9, 85, 16, 0, 0, 0)
    for k, v in kwargs.items():
        assert hasattr(params, k), 'No encoder_param named {}'.format(k)
        setattr(params, k, v)
    return params


def cwipc_get_version_module() -> str:
    c_version = cwipc_codec_dll_load().cwipc_get_version_codec()
    version = c_version.decode('utf8')
    return version

def cwipc_new_encoder(version : Optional[int]=None, params : Union[dict[str,Any], cwipc_encoder_params, None]=None, **kwargs :  Any) -> cwipc_encoder_wrapper:
    if version == None:
        version = CWIPC_ENCODER_PARAM_VERSION
    if isinstance(params, cwipc_encoder_params):
        assert not kwargs
    elif isinstance(params, dict):
        params = cwipc_new_encoder_params(**params)
        assert not kwargs
    else:
        params = cwipc_new_encoder_params(**kwargs)
    errorString = ctypes.c_char_p()
    obj = cwipc_codec_dll_load().cwipc_new_encoder(version, params, ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not obj:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if obj:
        return cwipc_encoder_wrapper(obj)
    raise CwipcError("cwipc_new_encoder: failed, but no specific error returned from C library")

def cwipc_new_encodergroup() -> cwipc_encodergroup_wrapper:
    errorString = ctypes.c_char_p()
    obj = cwipc_codec_dll_load().cwipc_new_encodergroup(ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not obj:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if obj:
        return cwipc_encodergroup_wrapper(obj)
    raise CwipcError("cwipc_new_encodergroup: failed, but no specific error returned from C library")
    
def cwipc_new_decoder() -> cwipc_decoder_wrapper:
    errorString = ctypes.c_char_p()
    obj = cwipc_codec_dll_load().cwipc_new_decoder(ctypes.byref(errorString), CWIPC_API_VERSION)
    if errorString and errorString.value and not obj:
        raise CwipcError(errorString.value.decode('utf8'))
    if errorString and errorString.value:
        warnings.warn(errorString.value.decode('utf8'))
    if obj:
        return cwipc_decoder_wrapper(obj)
    raise CwipcError("cwipc_new_decoder: failed, but no specific error returned from C library")
