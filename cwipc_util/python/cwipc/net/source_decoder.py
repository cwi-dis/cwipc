import time
import os
import socket
import threading
import queue
from typing import Optional, List, Union, Any, cast
try:
    from typing import override
except ImportError:
    from typing_extensions import override

from cwipc.abstract import cwipc_tileinfo_dict
from .abstract import cwipc_activesource_abstract, cwipc_source_abstract, cwipc_rawsource_abstract, cwipc_activerawsource_abstract, cwipc_pointcloud_abstract

try:
    from .. import codec
except ModuleNotFoundError:
    codec = None
FOURCC = "cwi1"

class _NetDecoder(threading.Thread, cwipc_activesource_abstract):
    """A source that decodes pointclouds gotten from a rawsource."""
    
    QUEUE_WAIT_TIMEOUT=1
    output_queue : queue.Queue[Optional[cwipc_pointcloud_abstract]]

    def __init__(self, source : cwipc_activerawsource_abstract, verbose : bool=False):
        threading.Thread.__init__(self)
        self.name = 'cwipc_util._NetDecoder'
        self.source = source
        self.source.set_fourcc(FOURCC)
        self.running = False
        self.verbose = verbose
        self.output_queue = queue.Queue(maxsize=2)
        self.times_decode = []
        self.pointcounts = []
        self.streamNumber = None
        self.decomp = None
        
    @override
    def free(self) -> None:
        pass
        
    @override
    def start(self, startsource : bool = True) -> bool:
        assert not self.running
        if self.verbose: print('netdecoder: start', flush=True)
        self.running = True
        threading.Thread.start(self)
        if startsource:
            didStart = self.source.start()
            return didStart
        else:
            return True
        
    @override
    def stop(self) -> None:
        if self.verbose: print('netdecoder: stop', flush=True)
        self.running = False
        self.source.stop()
        try:
            self.output_queue.put(None, block=False)
        except queue.Full:
            pass
        self.join()
        
    @override
    def eof(self) -> bool:
        return not self.running or self.output_queue.empty() and self.source.eof()
    
    @override
    def available(self, wait : bool=False) -> bool:
        # xxxjack if wait==True should get and put
        if not self.running:
            return False
        if not self.output_queue.empty():
            return True
        return self.source.available(wait)
        
    @override
    def get(self) -> Optional[cwipc_pointcloud_abstract]:
        if self.eof():
            return None
        pc = self.output_queue.get()
        return pc
        
    def run(self) -> None:
        if self.verbose: print(f"netdecoder: thread started", flush=True)
        while self.running:
            if self.source.eof():
                break
            cpc = self.source.get()
            if not cpc:
                if not self.source.eof():
                    print(f'netdecoder: source.get returned no data, but not eof()', flush=True)
                break
            t1 = time.time()
            pc = self._decompress(cpc)
            assert pc
            t2 = time.time()
            self.times_decode.append(t2-t1)
            self.output_queue.put(pc)
            pointcount = pc.count()
            self.pointcounts.append(pointcount)
            if self.verbose: print(f'netdecoder: decoded pointcloud with {pointcount} points, qlen={self.output_queue.qsize()}', flush=True)
        if self.verbose: print(f"netdecoder: thread exiting", flush=True)
        self.running = False

    def _decompress(self, cpc : bytes) -> Optional[cwipc_pointcloud_abstract]:
        if self.decomp == None:
            assert codec
            self.decomp = codec.cwipc_new_decoder()
        self.decomp.feed(cpc)
        gotData = self.decomp.available(True)
        if not gotData: return None
        pc = self.decomp.get()
        return pc

    @override
    def statistics(self) -> None:
        self.print1stat('decode_time', self.times_decode)
        self.print1stat('decode_count', self.pointcounts, isInt=True)
        if hasattr(self.source, 'statistics'):
            self.source.statistics()
        
    def print1stat(self, name : str, values : List[Union[int, float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print('netdecoder: {}: count=0'.format(name))
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = 'netdecoder: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = 'netdecoder: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(name, count, avgValue, minValue, maxValue))

    @override
    def request_metadata(self, name: str) -> None:
        assert False

    @override
    def is_metadata_requested(self, name: str) -> bool:
        return False

    @override
    def reload_config(self, config: str | bytes | None) -> None:
        raise NotImplementedError

    @override
    def reload_config_fast(self, config: str | bytes) -> None:
        raise NotImplementedError

    @override
    def get_config(self) -> bytes:
        raise NotImplementedError

    @override
    def seek(self, timestamp: int) -> bool:
        raise NotImplementedError

    @override
    def auxiliary_operation(self, op: str, inbuf: bytes, outbuf: bytearray) -> bool:
        raise NotImplementedError

    @override
    def maxtile(self) -> int:
        raise NotImplementedError

    @override
    def get_tileinfo_dict(self, tilenum: int) -> dict[str, Any]:
        raise NotImplementedError

    
def cwipc_activesource_decoder(source : cwipc_activerawsource_abstract, verbose : bool=False) -> cwipc_activesource_abstract:
    """Return cwipc_source-like object that reads compressed pointclouds from another source and decompresses them"""
    if codec == None:
        raise RuntimeError("netdecoder requires cwipc.codec which is not available")
    rv = _NetDecoder(source, verbose=verbose)
    return rv
        
    
def cwipc_source_decoder(source : cwipc_rawsource_abstract, verbose : bool=False) -> cwipc_source_abstract:
    """Return cwipc_source-like object that reads compressed pointclouds from another source and decompresses them"""
    if codec == None:
        raise RuntimeError("netdecoder requires cwipc.codec which is not available")
    rv = _NetDecoder(cast(cwipc_activerawsource_abstract, source), verbose=verbose)
    ok = rv.start(startsource=False)
    if not ok:
        print(f"netdecoder: could not start decoder")
    return rv
        
