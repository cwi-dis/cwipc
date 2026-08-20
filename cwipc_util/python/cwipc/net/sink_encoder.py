import threading
import time
import queue
import cwipc
import cwipc.codec
from typing import Optional, List, Any
from .abstract import *

DEFAULT_OCTREE_BITS=9
DEFAULT_JPEG_QUALITY=85
class _Sink_Encoder(threading.Thread, cwipc_sink_abstract):
    """A pointcloud sink that compresses pointclouds and forwards them to a rawsink."""
    
    FOURCC="cwi1"
    SELECT_TIMEOUT=0.1
    QUEUE_FULL_TIMEOUT=0.001

    sink : cwipc_rawsink_abstract
    input_queue : queue.Queue[Optional[cwipc.cwipc_pointcloud_wrapper]]
    pointcounts : List[int]
    packetsizes : List[int]
    tiledescriptions : List[cwipc.cwipc_tileinfo_dict]
    encoder_group : Optional[cwipc.codec.cwipc_encodergroup_wrapper]
    encoders : List[cwipc.codec.cwipc_encoder_wrapper]
    times_encode : List[float]

    # xxxjack the Any for sink is a cop-out. Need to define ABCs for all the types.
    def __init__(self, sink : cwipc_rawsink_abstract, verbose : bool=False, nodrop : bool=False):
        threading.Thread.__init__(self)
        self.name = 'cwipc_util._Sink_Encoder'
        self.sink = sink
        self.sink.set_fourcc(self.FOURCC)
        self.producer = None
        self.nodrop = nodrop
        self.input_queue = queue.Queue(maxsize=2)
        self.verbose = verbose
        self.nodrop = nodrop
        self.stopped = False
        self.started = False
        self.times_encode = []
        self.pointcounts = []
        self.packetsizes = []
        self.encoder_group = None
        self.encoders = []
        
        self.tiledescriptions = [{}]
        self.octree_bits : List[int] = [DEFAULT_OCTREE_BITS]
        self.jpeg_quality : List[int] = [DEFAULT_JPEG_QUALITY]
        
    def set_encoder_params(self, tiles : List[cwipc.cwipc_tileinfo_dict], octree_bits : int|List[int]|None = None, jpeg_quality : int|List[int]|None = None) -> None:
        if tiles == None: tiles = [{}]
        self.tiledescriptions = tiles
        if octree_bits == None:
            pass
        elif type(octree_bits) == int:
            self.octree_bits = [octree_bits]
        else:
            assert type(octree_bits) == list
            self.octree_bits = octree_bits
        if jpeg_quality == None:
            pass
        elif type(jpeg_quality) == int:
            self.jpeg_quality = [jpeg_quality]
        else:
            assert type(jpeg_quality) == list
            self.jpeg_quality = jpeg_quality
        
    def start(self) -> None:
        self._init_encoders()
        threading.Thread.start(self)
        self.sink.start()
        self.started = True
        
    def stop(self) -> None:
        if self.verbose: print(f"encoder: stopping thread")
        self.stopped = True
        try:
            self.input_queue.put(None, block=False)
        except queue.Full:
            pass
        self.sink.stop()
        if self.started:
            self.join()
        
    def set_producer(self, producer : Any) -> None:
        self.producer = producer
        self.sink.set_producer(producer)
        
    def is_alive(self):
        return not self.stopped  
        
    def run(self):
        assert self.encoder_group
        if self.verbose: print(f"encoder: thread started")
        try:
            while not self.stopped and self.producer and self.producer.is_alive():
                pc = self.input_queue.get()
                if not pc:
                    if not self.stopped:
                        print(f"encoder: get() returned None")
                    continue
                self.pointcounts.append(pc.count())
                
                t1 = time.time()
                self.encoder_group.feed(pc)
                packets : List[bytearray] = []
                for i in range(len(self.encoders)):
                    got_data = self.encoders[i].available(True)
                    assert got_data
                    cpc = self.encoders[i].get_bytes()
                    self.packetsizes.append(len(cpc))
                    packets.append(cpc)
                t2 = time.time()
                if self.verbose:
                    print(f"encoder: encoded {len(self.encoders)} times in {t2-t1:.3f}")
                
                if len(packets) == 1:
                    self.sink.feed(packets[0])
                else:
                    for i in range(len(packets)):
                        self.sink.feed(packets[i], stream_index=i)
                pc = None
                self.times_encode.append(t2-t1)
        finally:
            self.stopped = True
            self.sink.stop()
            if self.verbose: print(f"encoder: thread stopped")
        
    def feed(self, pc : cwipc.cwipc_pointcloud_wrapper) -> None:
        try:
            if self.nodrop:
                self.input_queue.put(pc)
            else:
                self.input_queue.put(pc, timeout=self.QUEUE_FULL_TIMEOUT)
        except queue.Full:
            if self.verbose: print(f"encoder: queue full")
    
    def _init_encoders(self):
        assert self.octree_bits
        assert self.jpeg_quality
            
        voxelsize = 0
        
        if self.verbose:
            print(f'encoder: creating {len(self.tiledescriptions)*len(self.octree_bits)*len(self.jpeg_quality)} encoders/streams')
        
        self.encoder_group = cwipc.codec.cwipc_new_encodergroup()
        for tileIdx in range(len(self.tiledescriptions)):
            for octree_bits in self.octree_bits:
                for jpeg_quality in self.jpeg_quality:
                    srctile = self.tiledescriptions[tileIdx].get("cameraMask", 0)
                    if self.verbose:
                        print(f"encoder: {len(self.encoders)}: tile={srctile}, octree_bits={octree_bits}, jpeg_quality={jpeg_quality}")
                    encparams = cwipc.codec.cwipc_encoder_params(False, 1, 1.0, octree_bits, jpeg_quality, 16, srctile, voxelsize)
                    encoder = self.encoder_group.addencoder(params=encparams)
                    self.encoders.append(encoder)
                    qualityDescription : cwipc_quality_description = dict(
                        octree_bits=octree_bits,
                        jpeg_quality=jpeg_quality
                    )
                    streamNum = self.sink.add_stream(tileIdx, self.tiledescriptions[tileIdx], qualityDescription)
                    if self.verbose:
                        print(f'encoder: streamNum={streamNum}, tile={tileIdx}, srctile={srctile}, octree_bits={octree_bits}, jpeg_quality={jpeg_quality}')

    def statistics(self):
        self.print1stat('encode_duration', self.times_encode)
        self.print1stat('encode_count', self.pointcounts, isInt=True)
        self.print1stat('packetsize', self.packetsizes, isInt=True)

        if hasattr(self.sink, 'statistics'):
            self.sink.statistics()
        
    def print1stat(self, name, values, isInt=False):
        count = len(values)
        if count == 0:
            print('encoder: {}: count=0'.format(name))
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = 'encoder: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = 'encoder: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(name, count, avgValue, minValue, maxValue))

def cwipc_sink_encoder(sink : cwipc_rawsink_abstract, verbose : bool=False, nodrop : bool=False) -> cwipc_sink_abstract:
    """Create a cwipc_sink object that compresses pointclouds and forward them to a rawsink."""
    if cwipc.codec == None:
        raise RuntimeError("cwipc_sink_encoder: requires cwipc.codec with is not available")
    return _Sink_Encoder(sink, verbose=verbose, nodrop=nodrop)