import threading
import socket
import select
import time
import queue
import cwipc
import cwipc.codec
import struct
from typing import Optional, List, Union
from .abstract import *

class _Sink_NetServer(threading.Thread, cwipc_rawsink_abstract):
    
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

    def __init__(self, port : int, verbose : bool=False, nodrop : bool=False, nonblocking : bool = False):
        threading.Thread.__init__(self)
        self.name = 'cwipc_util._Sink_NetServer'
        self.producer = None
        self.input_queue = queue.Queue(maxsize=2)
        self.verbose = verbose
        self.nodrop = nodrop
        self.nonblocking = nonblocking
        self.stopped = False
        self.started = False
        self.fourcc = None
        self.port = port
        self.dropcount = 0
        self.times_forward = []
        self.sizes_forward = []
        self.bandwidths_forward = []
        self.stream_added : bool = False
        self.socket = socket.socket()
        self.socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.socket.bind(('', self.port))
        self.socket.listen()
        if self.verbose:
            print(f"netserver: listen on :{self.port}")
        self.conn_sockets = []
        self.encoder = self._init_encoder()

    def _init_encoder(self) -> cwipc.codec.cwipc_encoder_wrapper:
        encparams = cwipc.codec.cwipc_encoder_params(False, 1, 1.0, 9, 85, 16, 0, 0)
        enc = cwipc.codec.cwipc_new_encoder(params=encparams)
        return enc
      
    def start(self) -> None:
        threading.Thread.start(self)
        self.started = True
        
    def stop(self) -> None:
        if self.verbose: print(f"netserver: stopping thread")
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
        
    def run(self):
        if self.verbose: print(f"netserver: thread started")
        try:
            while not self.stopped and self.producer and self.producer.is_alive():
                #
                # First check if there is a new incoming connection (waiting if we have no active connection sockets)
                #
                select_timeout = self.SELECT_LONG_TIMEOUT
                if len(self.conn_sockets) > 0:
                    select_timeout = self.SELECT_TIMEOUT
                try:
                    # We don't care about writeable conn_sockets, except that we want select to return quickly if there are any
                    readable, _, errorable = select.select([self.socket], self.conn_sockets, [], select_timeout)
                except socket.error:
                    continue
                if self.socket in errorable:
                    break
                if self.socket in readable:
                    connSocket, other = self.socket.accept()
                    if self.verbose:
                        print(f"netserver: accepted connection on :{self.port} from {other}")
                    self.conn_sockets.append(connSocket)
                #
                # Next transmit the data over all connection sockets
                #
                if self.conn_sockets:
                    t1 = time.time()
                    data = self.input_queue.get()
                    if data == None and self.stopped:
                        break
                    assert data != None
                    hdr = self._gen_header(data)
                    packet = hdr + data
                    conn_socket_error : List[socket.socket]
                    conn_socket_writeable : List[socket.socket]
                    #
                    # If we have a single outgoing connection we wait indefinitely, if we have multiple
                    # outgoing connections we only transmit on those that can transmit, too bad for the
                    # other connections.
                    #
                    select_timeout = self.SELECT_TIMEOUT
                    if len(self.conn_sockets) <= 1:
                        select_timeout = self.SELECT_LONG_TIMEOUT
                    try:
                        _, conn_socket_writeable, conn_socket_error = select.select([], self.conn_sockets, self.conn_sockets, select_timeout)
                    except socket.error:
                        continue
                    
                    for connSocket in conn_socket_writeable:
                        peerName = 'unknown'
                        try:
                            peerName = connSocket.getpeername()
                            connSocket.sendall(packet)
                        except (socket.error, ConnectionError) as err:
                            conn_socket_error.append(connSocket)
                            if self.verbose:
                                print(f"netserver: error on send to {peerName}: {err}")
                    t2 = time.time()
                    if self.verbose:
                        print(f"netserver: transmitted {len(hdr+data)} bytes on {len(self.conn_sockets)} connections")
                    if t2 == t1: t2 = t1 + 0.0005
                    self.times_forward.append(t2-t1)
                    datasize = len(data)
                    self.sizes_forward.append(datasize)
                    self.bandwidths_forward.append(datasize/(t2-t1))
                    for connSocket in conn_socket_error:
                        connSocket.close()
                        if self.verbose: print(f"netserver: connection closed")
                        self.conn_sockets.remove(connSocket)
        finally:
            self.stopped = True
            if self.verbose: print(f"netserver: thread stopped")
            for s in self.conn_sockets:
                s.close()
            
    
    def _gen_header(self, data : bytes) -> bytes:
        assert self.fourcc
        datalen = len(data)
        timestamp = int(time.time() * 1000)
        return struct.pack("=LLQ", VRT_4CC(self.fourcc), datalen, timestamp)
    
    def feed(self, data : bytes) -> bool:
        try:
            if self.nodrop:
                self.input_queue.put(data)
            else:
                self.input_queue.put(data, block=not self.nonblocking, timeout=self.QUEUE_FULL_TIMEOUT)
        except queue.Full:
            if self.verbose: print(f"netserver: queue full, drop packet")
            self.dropcount += 1
            return False
        return True           
    
    def _encode_pc(self, pc : cwipc.cwipc_pointcloud_wrapper) -> bytes:
        enc = self.encoder
        assert enc
        enc.feed(pc)
        gotData = enc.available(True)
        assert gotData
        data = enc.get_bytes()
        return data

    def statistics(self) -> None:
        self.print1stat('connection_duration', self.times_forward)
        self.print1stat('packetsize', self.sizes_forward, isInt=True)
        self.print1stat('bandwidth', self.bandwidths_forward)
        
    def print1stat(self, name : str, values : Union[List[int], List[float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print('netserver: {}: port={}, dropped={}, count=0'.format(name, self.port, self.dropcount))
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = 'netserver: {}: port={}, dropped={}, count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = 'netserver: {}: port={}, dropped={}, count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(name, self.port, self.dropcount, count, avgValue, minValue, maxValue))

    def add_stream(self, tilenum: Optional[int] = None, tiledesc: Optional[cwipc_tileinfo_dict] = None, qualitydesc: Optional[cwipc_quality_description] = None) -> int:
        # We ignore the arguments: there's nothing we can do with them anyway.
        if self.stream_added:
            raise RuntimeError("netserver: only single stream supported")
        self.stream_added = True
        return 0


class _Sink_MultiNetServer(cwipc_rawsink_abstract):
    def __init__(self, port : int, nstream : int, verbose : bool=False, nodrop : bool=False):
        if nodrop:
            raise RuntimeError("netserver: cannot use nodrop with multiple streams")
        self.streams : List[_Sink_NetServer] = [
            _Sink_NetServer(port+i, verbose=verbose, nonblocking=True)
            for i in range(nstream)
        ]
        self.n_stream_added : int = 0

    def add_stream(self, tilenum: Optional[int] = None, tiledesc: Optional[cwipc_tileinfo_dict] = None, qualitydesc: Optional[cwipc_quality_description] = None) -> int:
        # We ignore the arguments: there's nothing we can do with them anyway.
        rv = self.n_stream_added
        self.n_stream_added += 1
        return rv

    def start(self) -> None:
        assert self.n_stream_added == len(self.streams)
        for s in self.streams:
            s.start()

    def stop(self) -> None:
        for s in self.streams:
            s.stop()

    def feed(self, data : bytes, stream_index : int = 0) -> bool:
        s = self.streams[stream_index]
        return s.feed(data)
    
    def set_fourcc(self, fourcc : vrt_fourcc_type) -> None:
        for s in self.streams:
            s.set_fourcc(fourcc)

    def set_producer(self, producer : cwipc_producer_abstract) -> None:
        for s in self.streams:
            s.set_producer(producer)

    def statistics(self) -> None:
        for s in self.streams:
            s.statistics()
    
def cwipc_sink_netserver(port : int, verbose : bool=False, nodrop : bool=False, nstream : int=1) -> cwipc_rawsink_abstract:
    """Create a cwipc_sink object that serves compressed pointclouds on a TCP network port"""
    if nstream == 1:
        return _Sink_NetServer(port, verbose=verbose, nodrop=nodrop)
    else:
        return _Sink_MultiNetServer(port, nstream, verbose=verbose, nodrop=nodrop)
