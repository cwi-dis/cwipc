import ctypes
import ctypes.util
import sys
import os
import time
import urllib.parse
from typing import Optional, Any, List, Union, Tuple
from .abstract import *

_lldpkg_dll_reference = None

LLDASH_PACKAGER_API_VERSION = 0x20250724

class LLDashPackagerError(RuntimeError):
    pass

class lldpkg_handle_p(ctypes.c_void_p):
    pass
    
class FrameInfo(ctypes.Structure):
    _fields_ = [
        ("timestamp", ctypes.c_longlong)
    ]
    
class streamDesc(ctypes.Structure):
    _fields_ = [
        ("MP4_4CC", ctypes.c_uint32),
        ("tileNumber", ctypes.c_uint32), # official DASH: objectX. Re-targeted for pointclouds
        ("x", ctypes.c_uint32), # official DASH: objectY. Re-targeted for pointclouds
        ("y", ctypes.c_uint32), # Official DASH: objectWidth. Retargeted for pointclouds
        ("z", ctypes.c_uint32), # Official DASH: objectHeight. Retargeted for pointclouds
        ("totalWidth", ctypes.c_uint32),
        ("totalHeight", ctypes.c_uint32),
    ]

    def __init__(self, fourcc : vrt_fourcc_type, *args : Any):
        super(streamDesc, self).__init__(VRT_4CC(fourcc), *args)
    
LLDashPackagerErrorCallbackType = ctypes.CFUNCTYPE(None, ctypes.c_char_p, ctypes.c_int)

def _lldpkg_dll(libname : Optional[str]=None) -> ctypes.CDLL:
    global _lldpkg_dll_reference
    if _lldpkg_dll_reference: return _lldpkg_dll_reference
    
    if libname == None:
        # Current preferred use: SIGNALS_SMD_PATH points to the right directory.
        dirname = os.environ.get('SIGNALS_SMD_PATH')
        if dirname:
            libname = os.path.join(dirname, 'lldash_packager.so')
        if not libname:
            libname = ctypes.util.find_library('lldash_packager.so')
            if not libname:
                libname = ctypes.util.find_library('lldash_packager')
            if not libname:
                raise LLDashPackagerError('Dynamic library lldash_packager not found. Set SIGNALS_SMD_PATH to the directory containing the lldash_packager library')
    assert libname
    # Signals library needs to be able to find some data files stored next to the DLL.
    # Tell it where they are.
    if os.path.isabs(libname) and not 'SIGNALS_SMD_PATH' in os.environ:
        libdirname = os.path.dirname(libname)
        os.putenv('SIGNALS_SMD_PATH', libdirname)
    _lldpkg_dll_reference = ctypes.cdll.LoadLibrary(libname)
    
    _lldpkg_dll_reference.lldpkg_create.argtypes = [ctypes.c_char_p, LLDashPackagerErrorCallbackType, ctypes.c_int, ctypes.c_int, ctypes.POINTER(streamDesc), ctypes.c_char_p, ctypes.c_int, ctypes.c_int, ctypes.c_uint64]
    _lldpkg_dll_reference.lldpkg_create.restype = lldpkg_handle_p
    
    _lldpkg_dll_reference.lldpkg_destroy.argtypes = [lldpkg_handle_p, ctypes.c_bool]
    _lldpkg_dll_reference.lldpkg_destroy.restype = None
    
    _lldpkg_dll_reference.lldpkg_push_buffer.argtypes = [lldpkg_handle_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_size_t]
    _lldpkg_dll_reference.lldpkg_push_buffer.restype = ctypes.c_bool
    
    
    _lldpkg_dll_reference.lldpkg_get_media_time.argtypes = [lldpkg_handle_p, ctypes.c_int, ctypes.c_int]
    _lldpkg_dll_reference.lldpkg_get_media_time.restype = ctypes.c_int64
    
    _lldpkg_dll_reference.lldpkg_get_version.argtypes = []
    _lldpkg_dll_reference.lldpkg_get_version.restype = ctypes.c_char_p
    
    return _lldpkg_dll_reference
 
class _LLDashPackagerSink(cwipc_rawsink_abstract):
    """A DASH sink that streams multiple data streams to a MotionSpell DASH ingestion server.
    
    Uses the lldash_packager native implementation under the hood.
    """
    
    streamDescs : Optional[List[streamDesc]]
    dll : ctypes.CDLL
    fourcc : Optional[vrt_fourcc_type]
    sizes_forward : List[int]

    def __init__(self, url : str="", *, verbose : bool=False, nodrop : bool=False, streamDescs : Optional[List[streamDesc]]=None, fourcc : Optional[vrt_fourcc_type]=None, seg_dur_in_ms : Optional[int]=None, timeshift_buffer_depth_in_ms : Optional[int]=None):
        if seg_dur_in_ms == None: seg_dur_in_ms=10000
        if timeshift_buffer_depth_in_ms == None: timeshift_buffer_depth_in_ms=30000
        self.verbose = verbose
        self.nodrop = nodrop
        self.url = url
        self.handle = None
        self.dll = _lldpkg_dll()
        assert self.dll
        self.url = url
        self.streamDescs = streamDescs
        self.fourcc = fourcc
        self.seg_dur_in_ms = seg_dur_in_ms
        self.timeshift_buffer_depth_in_ms = timeshift_buffer_depth_in_ms
        self.handle = None        
        self.sizes_forward = []
        self.bandwidths_forward = []
        lldash_log_setting = os.environ.get("LLDASH_LOGGING", None)
        self.lldash_logging = not not lldash_log_setting
        self._onLLDashPackagerError = LLDashPackagerErrorCallbackType(self._onLLDashPackagerError)
        if self.verbose:
            print(f"lldash_packager: native library version: {self.dll.lldpkg_get_version().decode('utf8')}", file=sys.stderr, flush=True)
        self.lldash_log(event="sink_lldpkg_init", version=self.dll.lldpkg_get_version().decode('utf8'))
        
    def __del__(self):
        self.free(force=True)
        
    def _onLLDashPackagerError(self, cmsg : bytes, level : int):
        """Callback function passed to vrt_create: preint (or re-raise) lldash_packager errors in Python environment"""
        msg = cmsg.decode('utf8')
        levelName = {0:"error", 1:"warning", 2:"info message", 3:"debug message"}.get(level, f"level-{level} message")
    
        print(f"lldash_packager: asynchronous {levelName}: {msg}", file=sys.stderr, flush=True)
        self.lldash_log(event="async_log_message", level=level, msg=msg)
        # xxxjack raising an exception from a ctypes callback doesn't work.
        # Need to fix at some point.
        if False and level == 0:
            raise SubError(msg)
                
    def lldash_log(self, **kwargs):
        if not self.lldash_logging:
            return
        lts = time.strftime("%Y-%m-%dT%H:%M:%S", time.localtime()) + f".{int(time.time()*1000)%1000:03d}"
        message = f"sink_lldpkg: ts={lts}"
        for k, v in kwargs.items():
            message += f" {k}={v}"
        print(message)
        sys.stdout.flush()
        
    def start(self) -> None:
        url = self.url.encode('utf8')
        if self.streamDescs == None:
            # Invent a streamDesc
            if self.fourcc == None:
                self.fourcc = VRT_4CC("cwi1")
            else:
                self.fourcc = VRT_4CC(self.fourcc)
            self.add_stream(0)
        assert(self.streamDescs)
        # ctypes array constructors are a bit weird. Check the documentation.
        streamDescCount = len(self.streamDescs)
        c_streamDescs = (streamDesc*streamDescCount)(*self.streamDescs)
        baseurl, mpdname = self._urlsplit(url.decode('utf8'))
        if self.verbose:
            for i in range(streamDescCount):
                print(f"sink_lldpkg: streamDesc[{i}]: MP4_4CC={c_streamDescs[i].MP4_4CC.to_bytes(4, 'big')}={c_streamDescs[i].MP4_4CC}, tileNumber={c_streamDescs[i].tileNumber}, x={c_streamDescs[i].x}, y={c_streamDescs[i].y}, z={c_streamDescs[i].z}, totalWidth={c_streamDescs[i].totalWidth}, totalHeight={c_streamDescs[i].totalHeight}")
        self.lldash_log(event="lldpkg_create_call", url=baseurl, seg_dur=self.seg_dur_in_ms, timeshift_buffer_depth=self.timeshift_buffer_depth_in_ms, streamDescCount=streamDescCount)
        msgLevel = 3 if self.verbose else 0
        self.handle = self.dll.lldpkg_create(mpdname.encode('utf8'), self._onLLDashPackagerError, msgLevel, streamDescCount, c_streamDescs, baseurl.encode('utf8'), self.seg_dur_in_ms, self.timeshift_buffer_depth_in_ms, LLDASH_PACKAGER_API_VERSION)
        self.lldash_log(event="lldpkg_create_returned", url=self.url)
        if not self.handle:
            raise LLDashPackagerError(f"lldpkg_create({url}) failed")

        assert self.handle
        
    def _urlsplit(self, url : str) -> Tuple[str, str]:
        split = urllib.parse.urlsplit(url)
        path = split.path
        basepath, mpdname = os.path.split(path)
        if basepath and basepath[-1] != '/':
            basepath = basepath + '/'
        if not mpdname:
            mpdname = "cwipc_lldpkg.mpd"
        mpdbasename, ext = os.path.splitext(mpdname)
        if ext != ".mpd":
            raise LLDashPackagerError(f"lldash_packager: URL {url} does not end with .mpd")
        baseurl = urllib.parse.urlunsplit((split.scheme, split.netloc, basepath, split.query, split.fragment))
        return baseurl, mpdbasename
        
    def stop(self) -> None:
        self.free(force=True)
        
    def set_producer(self, producer : cwipc_producer_abstract) -> None:
        pass
        
    def set_fourcc(self, fourcc : vrt_fourcc_type) -> None:
        self.fourcc = fourcc
        
    def _set_streamDescs(self, streamDescs : List[streamDesc]) -> None:
        self.streamDescs = streamDescs
        

    def add_stream(self, tilenum: Optional[int] = None, tiledesc: Optional[cwipc_tileinfo_dict] = None, qualitydesc: Optional[cwipc_quality_description] = None) -> int:
        if not self.streamDescs:
            self.streamDescs = []
        if tilenum == None or tiledesc == None:
            raise RuntimeError("sink_lldpkg: add_stream: tilenum and tiledesc are required")
        # We ignore qualitydesc, for now. Will investigate whether we can pass these in the fields we still have available in the MPD
        normal = tiledesc.get("normal", dict(x=0,y=0,z=0))
        x = normal['x']
        y = normal['y']
        z = normal['z']
        if type(x) != int: x = int(x*1000)
        if type(y) != int: y = int(y*1000)
        if type(z) != int: z = int(z*1000)
        assert self.fourcc
        self.streamDescs.append(streamDesc(self.fourcc, tilenum, x, y, z))
        return len(self.streamDescs)-1

    def free(self, *, force:bool=False) -> None:
        if self.handle:
            tmp_handle = self.handle
            self.handle = None
            assert self.dll
            self.lldash_log(event="lldpkg_destroy_call", url=self.url)
            self.dll.lldpkg_destroy(tmp_handle, True)
            self.lldash_log(event="lldpkg_destroy_returned", url=self.url)
            self.handle = None
            
    def feed(self, buffer : Union[bytes, bytearray], stream_index : int = 0) -> bool:
        if not self.handle:
            return False
        assert self.dll
        length = len(buffer)
        ok : bool
        self.lldash_log(event="lldpkg_push_buffer_call", url=self.url, stream_index=stream_index, length=length)
        ok = self.dll.lldpkg_push_buffer(self.handle, stream_index, bytes(buffer), length)
        self.lldash_log(event="lldpkg_push_buffer_returned", url=self.url)
            
        if ok:
            self.sizes_forward.append(length)
        else:
            raise LLDashPackagerError(f"lldpkg_push_buffer(handle, {stream_index}, buffer, {length}) failed")
        return ok

    def canfeed(self, timestamp : int, wait : bool=True) -> bool:
        return not not self.handle

    def statistics(self) -> None:
        self.print1stat('packetsize', self.sizes_forward)
        
    def print1stat(self, name : str, values : Union[List[int], List[float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print('lldash_packager: {}: count=0'.format(name))
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = 'lldash_packager: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = 'lldash_packager: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(name, count, avgValue, minValue, maxValue))

def cwipc_sink_lldpkg(url : str, verbose : bool=False, nodrop : bool=False, nstream : int = 1, **kwargs : Any) -> cwipc_rawsink_abstract:
    """Create a sink that transmits to a MotionSpell lldash ingestion server."""
    return _LLDashPackagerSink(url, verbose=verbose, nodrop=nodrop, **kwargs)