import threading
import socket
import time
import queue
import cwipc
import cwipc.codec
import struct
from typing import Optional, List, Union
from .abstract import *

class _Sink_NetIngest(threading.Thread, cwipc_rawsink_abstract):
    
    SELECT_TIMEOUT=0.1
    SELECT_LONG_TIMEOUT=5.0
    QUEUE_FULL_TIMEOUT=0.001
    
    producer : Optional[cwipc_producer_abstract]
    input_queue : queue.Queue[Optional[bytes]]
    times_forward : List[float]
    sizes_forward : List[int]
    bandwidths_forward : List[float]
    fourcc : Optional[vrt_fourcc_type]
    conn_sockets : List[socket.socket]

    def __init__(self, address : str, verbose : bool=False, nodrop : bool=False):
        threading.Thread.__init__(self)
        self.name = 'cwipc_util._Sink_NetIngest'
        self.producer = None
        self.input_queue = queue.Queue(maxsize=2)
        self.verbose = verbose
        self.nodrop = nodrop
        self.stopped = False
        self.started = False
        self.fourcc = None
        self.times_forward = []
        self.sizes_forward = []
        self.bandwidths_forward = []
        host, port_str = address.split(":")
        port = int(port_str)
        self.address = (host, port)
        self.stream_added = False
        self.encoder = self._init_encoder()
        self._open()

    def _init_encoder(self) -> cwipc.codec.cwipc_encoder_wrapper:
        encparams = cwipc.codec.cwipc_encoder_params(False, 1, 1.0, 9, 85, 16, 0, 0)
        enc = cwipc.codec.cwipc_new_encoder(params=encparams)
        return enc
    
    def start(self) -> None:
        threading.Thread.start(self)
        self.started = True
        
    def stop(self) -> None:
        if self.stopped:
            return
        if self.verbose: print(f"netingest: stopping thread")
        self.stopped = True
        self.socket.close()
        if self.input_queue:
            try:
                while self.input_queue.get(block=False):
                    pass
            except queue.Empty:
                pass
            self.input_queue.put(None)
        if self.started:
            self.join()
        self.encoder = None
        
    def set_fourcc(self, fourcc : vrt_fourcc_type) -> None:
        self.fourcc = fourcc

    def set_producer(self, producer : cwipc_producer_abstract) -> None:
        self.producer = producer
        
    def is_alive(self) -> bool:
        return not self.stopped  
        
    def _open(self) -> None:
        self.socket = socket.socket()
        try:
            self.socket.connect(self.address)
        except socket.error as e:
            print(f"netingest: connection failed: {e}")
            self.stopped = True
            raise

    def run(self):
        if self.verbose: print(f"netingest: thread started")

        try:
            while not self.stopped and self.producer and self.producer.is_alive():
                    t1 = time.time()
                    try:
                        data = self.input_queue.get(timeout=1)
                    except queue.Empty:
                        continue
                    if data == None and self.stopped:
                        break
                    assert data != None
                    hdr = self._gen_header(data)
                    packet = hdr + data
                
                    try:
                        self.socket.sendall(packet)
                    except socket.error:
                        if self.verbose:
                            print(f"netingest: error on send to {self.socket.getpeername()}")
                        break
                    t2 = time.time()
                    if self.verbose:
                        print(f"netingest: transmitted {len(hdr+data)} bytes on {len(self.conn_sockets)} connections")
                    if t2 == t1: t2 = t1 + 0.0005
                    self.times_forward.append(t2-t1)
                    datasize = len(data)
                    self.sizes_forward.append(datasize)
                    self.bandwidths_forward.append(datasize/(t2-t1))
        finally:
            self.stopped = True
            if self.verbose: print(f"netingest: thread stopping")
    
    def _gen_header(self, data : bytes) -> bytes:
        assert self.fourcc
        datalen = len(data)
        timestamp = int(time.time() * 1000)
        return struct.pack("=LLQ", VRT_4CC(self.fourcc), datalen, timestamp)
    
    def feed(self, data : bytes) -> None:
        try:
            if self.nodrop:
                self.input_queue.put(data)
            else:
                self.input_queue.put(data, timeout=self.QUEUE_FULL_TIMEOUT)
        except queue.Full:
            if self.verbose: print(f"netingest: queue full, drop packet")            
    
    def _encode_pc(self, pc : cwipc.cwipc_pointcloud_wrapper) -> bytes:
        enc = self.encoder
        assert enc
        enc.feed(pc)
        gotData = enc.available(True)
        assert gotData
        data = enc.get_bytes()
        return data

    def add_stream(self, tilenum: Optional[int] = None, tiledesc: Optional[cwipc_tileinfo_dict] = None, qualitydesc: Optional[cwipc_quality_description] = None) -> int:
        # We ignore the arguments: there's nothing we can do with them anyway.
        if self.stream_added:
            raise RuntimeError("netingest: only single stream supported")
        self.stream_added = True
        return 0

    def statistics(self) -> None:
        self.print1stat('connection_duration', self.times_forward)
        self.print1stat('packetsize', self.sizes_forward, isInt=True)
        self.print1stat('bandwidth', self.bandwidths_forward)
        
    def print1stat(self, name : str, values : Union[List[int], List[float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print('netingest: {}: count=0'.format(name))
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = 'netingest: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = 'netingest: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(name, count, avgValue, minValue, maxValue))

def cwipc_sink_netingest(address : str, verbose : bool=False, nodrop : bool=False, nstream : int = 1) -> cwipc_rawsink_abstract:
    """Create a cwipc_sink object that serves compressed pointclouds on a TCP network port"""
    if nstream == 1:
        return _Sink_NetIngest(address, verbose=verbose, nodrop=nodrop)
    else:
        raise RuntimeError("cwipc_sink_netingest: only single-stream supported")
