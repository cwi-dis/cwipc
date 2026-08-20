import time
import os
import socket
import threading
import queue
from typing import Optional, List, Union
try:
    from typing import override
except ImportError:
    from typing_extensions import override

from cwipc.abstract import cwipc_tileinfo_dict
from .abstract import *
from ..util import cwipc_join, cwipc_pointcloud_wrapper


class _Synchronizer(threading.Thread, cwipc_activesource_abstract):
    """A source that combines point clouds gotten from multiple (tiled) sources into one stream."""
    
    QUEUE_WAIT_TIMEOUT=1
    output_queue : queue.Queue[Optional[cwipc_pointcloud_abstract]]

    def __init__(self, reader : cwipc_activerawmultisource_abstract, sources : List[cwipc_source_abstract], verbose : bool=False):
        threading.Thread.__init__(self)
        self.name = 'cwipc_util._NetDecoder'
        self.reader = reader
        self.sources : List[cwipc_source_abstract] = sources
        self.n_tile = len(self.sources)
        self.input_buffers : List[Optional[cwipc_pointcloud_abstract]] = [None] * self.n_tile
        self.running = False
        self.verbose = verbose
        self.output_queue = queue.Queue(maxsize=6)
        self.prefer_partial_over_unsynced = True
        #self.streamNumber = None
        self.combine_times = []
        self.late_per_occurrence = []
        self.desync_per_occurrence = []
        self.missing_per_occurrence = []

    @override
    def free(self) -> None:
        pass
        
    @override
    def start(self) -> bool:
        assert not self.running
        if self.verbose: print('synchronizer: start', flush=True)
        self.running = True
        # Note: we don't start the sources ourselves: they are not activesources, they may
        # be decoders.
        #for s in self.sources:
        #    s.start()
        ok = self.reader.start()
        if not ok:
            return False
        threading.Thread.start(self)
        return True
    
    @override
    def stop(self) -> None:
        if self.verbose: print('synchronizer: stop', flush=True)
        self.running = False
        # See comment in start().
        # for s in self.sources:
        #    s.stop()
        self.reader.stop()
        try:
            self.output_queue.put(None, block=False)
        except queue.Full:
            pass
        self.join()
        
    @override
    def eof(self) -> bool:
        if not self.running:
            return True
        if not self.output_queue.empty():
            return False
        return self._any_source_eof()
    
    def _any_source_eof(self) -> bool:
        for s in self.sources:
            if s.eof():
                return True
        return False
    
    @override
    def available(self, wait : bool=False) -> bool:
        # xxxjack if wait==True should get and put
        if not self.running:
            return False
        if not self.output_queue.empty():
            return True
        for s in self.sources:
            if not s.available(wait=wait):
                return False
        return True
        
    @override
    def get(self) -> Optional[cwipc_pointcloud_abstract]:
        if self.eof():
            return None
        pc = self.output_queue.get()
        return pc
        
    def run(self) -> None:
        if self.verbose: print(f"synchronizer: thread started", flush=True)
        earliest_timestamp = 0
        latest_timestamp = 0
        while self.running:
            if self._any_source_eof():
                if self.verbose: print(f"synchronizer: end of file")
                break
            any_work_done = False
            # Compute the earliest timestamp (which is the latest of the currently available buffer heads)
            for buffer_head in self.input_buffers:
                if buffer_head:
                    ts = buffer_head.timestamp()
                    latest_timestamp = max(latest_timestamp, ts)
            # Throw away outdated heads
            for i in range(self.n_tile):
                buf = self.input_buffers[i]
                if buf:
                    if buf.timestamp() < earliest_timestamp:
                        self.input_buffers[i] = None
            # See whether any empty buffer slots can be filled, and see whether any empty ones remain
            any_empty_input_buffers = False
            for i in range(self.n_tile):
                if self.input_buffers[i] == None:
                    
                    if self.sources[i].available(False):
                        pc = self.sources[i].get()
                        if not pc:
                            print(f"synchronizer: source {i} returned no point cloud")
                            any_empty_input_buffers = True
                            break
                        if self.verbose:
                            print(f"synchronizer: got ts={pc.timestamp()} from {i}")
                        if pc.timestamp() >= earliest_timestamp:
                            self.input_buffers[i] = pc
                            any_work_done = True
                        else:
                            # Unfortunately this partial point cloud is too late
                            too_late = earliest_timestamp - pc.timestamp()
                            if self.verbose: print(f"synchronizer: tile {i}: too late by {too_late}")
                            self.late_per_occurrence.append(too_late)
                            any_empty_input_buffers = True
                    else:
                        any_empty_input_buffers = True
            # If not all buffers are filled yet we wait, and go through the loop once more.
            if any_empty_input_buffers:
                # if self.verbose: print(f"synchronizer: still missing pointclouds")
                time.sleep(0.001)
                continue
            current_timestamps = [pc.timestamp() for pc in self.input_buffers if pc]
            if self.verbose: print(f"synchronizer: all tiles pc={current_timestamps}")
            current_earliest_timestamp = min(current_timestamps)
            current_latest_timestamp = max(current_timestamps)
            # If we are okay with producing partial point clouds we keep only
            # the ones with the correct timestamp
            # The alternative is that we always produce a full point cloud, but
            # it may be de-synced (tiles with different timestamps)
            if self.prefer_partial_over_unsynced:
                to_combine = [pc for pc in self.input_buffers if pc and pc.timestamp() == current_earliest_timestamp]
                desync = 0
            else:
                to_combine = [pc for pc in self.input_buffers]
                desync = current_latest_timestamp - current_earliest_timestamp
            if len(to_combine) < self.n_tile:
                missing = self.n_tile - len(to_combine)
                self.missing_per_occurrence.append(missing)
            if desync > 0:
                self.desync_per_occurrence.append(desync)
            # Now we just need to combine the point clouds, and set a reasonable timestamp and cellsize
            result_pc : Optional[cwipc_pointcloud_wrapper] = None
            t0 = time.time()
            current_cellsize = min([pc.cellsize() for pc in to_combine if pc])
            for pc in to_combine:
                assert pc
                assert isinstance(pc, cwipc_pointcloud_wrapper)
                if result_pc == None:
                    result_pc = pc
                else:
                    result_pc = cwipc_join(result_pc, pc)
            t1 = time.time()
            assert result_pc
            result_pc._set_timestamp(current_earliest_timestamp)
            result_pc._set_cellsize(current_cellsize)
            latency = int(time.time()*1000) - current_earliest_timestamp
            earliest_timestamp = current_earliest_timestamp + 1
            self.combine_times.append(t1-t0)
            if self.verbose: print(f'synchronizer: produced pointcloud ts={result_pc.timestamp()} with {result_pc.count()} points, latency={latency} ms, qlen={self.output_queue.qsize()}', flush=True)
            self.output_queue.put(result_pc)

        if self.verbose: print(f"synchronizer: thread exiting", flush=True)
        self.running = False
        try:
            self.output_queue.put(None, block=False)
        except queue.Full:
            pass

    @override
    def statistics(self) -> None:
        self.print1stat('combine_time', self.combine_times)
        self.print1stat('late', self.late_per_occurrence)
        self.print1stat('desync', self.desync_per_occurrence)
        self.print1stat('missing', self.missing_per_occurrence, isInt=True)

        for s in self.sources:
            if hasattr(s, 'statistics'):
                s.statistics()
        
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
    
class _MQSynchronizer(_Synchronizer):
    """A source that combines point clouds gotten from multiple multi-quality sources into one stream."""
    
    def __init__(self, reader : cwipc_activerawmultisource_abstract, sources : List[cwipc_source_abstract], verbose : bool=False):
        super().__init__(reader, sources, verbose=verbose)
        self.description : cwipc_multistream_description = []
        self.current_quality : int = 0
        
    def select_next_tile_quality(self) -> Optional[str]:
        """Debug call: select the next quality"""
        if not self.description:
            self.description = self.reader.get_description()
        nQualities = len(self.description[0])
        self.current_quality = (self.current_quality + 1) % nQualities
        for tIdx in range(len(self.sources)):
            self.reader.select_tile_quality(tIdx, self.current_quality)
        return f"quality {self.current_quality} of {nQualities}"

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

        
def cwipc_source_synchronizer(reader : cwipc_activerawmultisource_abstract, sources : List[cwipc_source_abstract], verbose : bool=False) -> cwipc_activesource_abstract:
    """Return cwipc_source-like object that combines and synchronizes pointclouds from multiple sources"""
    rv = _MQSynchronizer(reader, sources, verbose=verbose)
    return rv
        
