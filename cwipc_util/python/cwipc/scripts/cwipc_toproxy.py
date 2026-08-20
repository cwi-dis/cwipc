"""
Send point cloud stream to cwipc_proxy API. Legacy.
"""
import sys
import os
import time
import threading
import traceback
import queue
import socket
import struct
from .. import CWIPC_POINT_PACKETHEADER_MAGIC, cwipc_pointcloud_wrapper
from ..net.abstract import cwipc_producer_abstract, cwipc_sink_abstract
from ._scriptsupport import *

class Sender(cwipc_sink_abstract):
    output_queue : queue.Queue[cwipc_pointcloud_wrapper]

    def __init__(self, host, port, verbose=False):
        self.producer = None
        self.output_queue = queue.Queue(maxsize=2)
        self.verbose = verbose
        # xxxjack should these move to start() and then also implement stop()?
        self.socket = socket.socket()
        self.socket.connect((host, port))

    def start(self) -> None:
        pass

    def stop(self) -> None:
        pass
        
    def set_producer(self, producer : cwipc_producer_abstract) -> None:
        self.producer = producer    
        
    def run(self):
        while self.producer and self.producer.is_alive():
            try:
                pc = self.output_queue.get(timeout=0.033)
                ok = self.send_pc(pc)
                pc = None
            except queue.Empty:
                pass
        
    def feed(self, pc : cwipc_pointcloud_wrapper) -> None:
        try:
            self.output_queue.put(pc, timeout=0.5)
        except queue.Full:
            pass
            
    def send_pc(self, pc : cwipc_pointcloud_wrapper) -> None:
        data = pc.get_bytes()
        cellsize = pc.cellsize()
        timestamp = pc.timestamp()
        header = struct.pack("<iiqfi", CWIPC_POINT_PACKETHEADER_MAGIC, len(data), timestamp, cellsize, 0)
        x = self.socket.send(header)
        y = self.socket.send(data)

    def statistics(self) -> None:
        pass

def main():
    SetupStackDumper()
    assert __doc__ is not None
    parser = ArgumentParser(description=__doc__.strip())
    parser.add_argument("host", action="store", help="Hostname where cwipc_proxy server is running")
    parser.add_argument("port", type=int, action="store", help="Port where cwipc_proxy server is running")
    args = parser.parse_args()

    sourceFactory = activesource_factory_from_args(args)
    source = sourceFactory()
    sender = Sender(args.host, args.port)
    sourceServer = SourceServer(source, sender, args)
    sourceThread = threading.Thread(target=sourceServer.run, args=())
    if sender:
        sender.set_producer(sourceThread)

    #
    # Run everything
    #
    try:
        sourceThread.start()

        if sender:
            sender.run()
            sourceServer.stop()
            
        sourceThread.join()
    except KeyboardInterrupt:
        print("Interrupted.")
        sourceServer.stop()
    except:
        traceback.print_exc()
    
    #
    # It is safe to call join (or stop) multiple times, so we ensure to cleanup
    #
    sourceServer.stop()
    sourceThread.join()
    sourceServer.statistics()
    
if __name__ == '__main__':
    main()
    
    
    
