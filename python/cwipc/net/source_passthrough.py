import time
import os
import socket
import threading
import queue
import cwipc
from typing import Optional, Union, List, Any, cast
try:
    from typing import override
except ImportError:
    from typing_extensions import override

from cwipc.abstract import cwipc_tileinfo_dict
from .abstract import cwipc_rawsource_abstract, cwipc_source_abstract,cwipc_activerawsource_abstract, cwipc_activesource_abstract, cwipc_pointcloud_abstract

try:
    import cwipc.codec
except ModuleNotFoundError:
    cwipc.codec = None
FOURCC = "cwi0"

class _NetPassthrough(threading.Thread, cwipc_activesource_abstract):
    
    QUEUE_WAIT_TIMEOUT=1
    
    source : cwipc_activerawsource_abstract
    output_queue : queue.Queue[Optional[cwipc_pointcloud_abstract]]

    def __init__(self, source : cwipc_activerawsource_abstract, verbose : bool=False):
        threading.Thread.__init__(self)
        self.name = 'cwipc_util._NetPassthrough'
        self.source = source
        self.source.set_fourcc(FOURCC)
        self.running = False
        self.verbose = verbose
        self.output_queue = queue.Queue(maxsize=2)
    
    @override
    def free(self) -> None:
        pass
    
    @override
    def start(self) -> bool:
        assert not self.running
        if self.verbose: print('passthrough: start')
        self.running = True
        threading.Thread.start(self)
        ok = self.source.start()
        return ok
        
    @override
    def stop(self) -> None:
        if self.verbose: print('passthrough: stop')
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
        if self.verbose: print(f"passthrough: thread started")
        while self.running:
            if not self.source.available(True):
                continue
            cpc = self.source.get()
            if not cpc:
                print(f'passthrough: source.get returned no data')
                self.output_queue.put(None)
                break
            pc = cwipc.cwipc_from_packet(cpc)
            self.output_queue.put(pc)
            if self.verbose: print(f'passthrough: deserialized pointcloud with {pc.count()} points timestamp={pc.timestamp()}')
        if self.verbose: print(f"passthrough: thread exiting")
        self.running = False

    @override
    def statistics(self) -> None:
        if hasattr(self.source, 'statistics'):
            self.source.statistics()
        
    def print1stat(self, name : str, values : Union[List[int], List[float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print('passthrough: {}: count=0'.format(name))
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = 'passthrough: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = 'passthrough: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
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
    def reload_config_fast(self, config: Union[str, bytes]) -> None:
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


    
def cwipc_activesource_passthrough(source : cwipc_activerawsource_abstract, verbose : bool=False) -> cwipc_activesource_abstract:
    """Return cwipc_source-like object that reads serialized (uncompressed) pointclouds from a rawsource and returns them"""
    rv = _NetPassthrough(source, verbose=verbose)
    return rv

def cwipc_source_passthrough(source : cwipc_rawsource_abstract, verbose : bool=False) -> cwipc_source_abstract:
    """Return cwipc_source-like object that reads serialized (uncompressed) pointclouds from a rawsource and returns them"""
    # The following cast _should_ be safe, because the unimplemented methods _should_ never be called.
    rv = _NetPassthrough(cast(cwipc_activerawsource_abstract, source), verbose=verbose)
    return rv
