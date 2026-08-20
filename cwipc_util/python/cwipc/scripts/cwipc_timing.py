"""
Get detailed timestamp information from a point cloud stream.
"""
import sys
import argparse
import threading
import traceback
import queue
import csv
from typing import Optional, Dict, Any, List, Iterable

from .. import cwipc_pointcloud_wrapper, cwipc_write, cwipc_write_debugdump, CwipcError, CWIPC_FLAGS_BINARY
from .. import codec
from ..util import cwipc_metadata
from ._scriptsupport import *
from ..net.abstract import *


class DropWriter(cwipc_sink_abstract):
    results : List[Dict[str, Any]]
    output_queue : queue.Queue[Optional[cwipc_pointcloud_wrapper]]
    csvwriter : Optional[csv.DictWriter]
    csvkeys : List[str]
    details : bool
    savergb : int
    savergb_counter : int
    output_filename : Optional[str]

    def __init__(self, args : argparse.Namespace, queuesize=5):
        self.producer = None
        self.output_queue = queue.Queue(maxsize=queuesize)
        self.count = 0
        self.details = args.details
        self.savergb = args.savergb
        self.savergb_counter = self.savergb
        self.output_filename = args.output
        self.results = []
        self.csvwriter = None
        self.csvkeys = []
        self.previous_timestamp = None
        
        
    def start(self) -> None:
        pass

    def stop(self) -> None:
        pass
        
    def set_producer(self, producer : cwipc_producer_abstract):
        self.producer = producer    
        
    def run(self):
        while (self.producer and self.producer.is_alive()) or not self.output_queue.empty():
            try:
                pc = self.output_queue.get(timeout=0.5)
                assert pc
                self.count = self.count + 1
                pc_timestamp = pc.timestamp()
                r : Dict[str, Any] = dict(
                    num=self.count,
                    timestamp=pc_timestamp,
                    pointcount=pc.count(),
                )
                if self.previous_timestamp != None:
                    r["frame_duration"] = pc_timestamp - self.previous_timestamp
                self.previous_timestamp = pc_timestamp
                metadata = pc.access_metadata()
                assert metadata
                r["aux"] = metadata.count()
                if metadata != None and metadata.count() > 0:
                    for i in range(metadata.count()):
                        auxname = metadata.name(i)
                        if not "timestamps" in auxname:
                            continue
                        descr_dict = metadata._parse_aux_description(metadata.description(i))
                        for k, v in descr_dict.items():
                            r[f"{auxname}.{k}"] = v
                        delta_time_depth = pc_timestamp - descr_dict["depth_timestamp"]
                        delta_time_color = pc_timestamp - descr_dict["color_timestamp"]
                        r[f"{auxname}.color_age"] = delta_time_color
                        r[f"{auxname}.depth_age"] = delta_time_depth

                    self.writerecord(r)
                    if self.savergb:
                        self.savergb_counter -= 1
                        if self.savergb_counter <= 0:
                            self.save_rgb(pc, metadata)
                            self.savergb_counter = self.savergb
                pc = None
                
            except queue.Empty:
                pass
        
        return True              
        
    def save_rgb(self, pc : cwipc_pointcloud_wrapper, metadata : cwipc_metadata) -> None:
        import cv2
        timestamp = pc.timestamp()
        image_dict = metadata.get_all_images("rgb.")
        for serial, image in image_dict.items():
            name = "rgb." + serial
            filename = f"{timestamp}.{serial}.png"
            # Jack is confused. I thought opencv always used BGR images, so those are returned from get_all_images()
            # But now it appears that imshow() wants BGR images but imwrite() wants RGB images?
            # Go figure...
            if False:
                swapped_image = image[:,:,[2,1,0]]
            else:
                swapped_image = image
            ok = cv2.imwrite(filename, swapped_image)
            if ok:
                print(f"wrote {filename}", file=sys.stderr)
            else:
                print(f"Error: failed to write {filename}", file=sys.stderr)
    
    def feed(self, pc : cwipc_pointcloud_wrapper) -> None:
        self.output_queue.put(pc)
        
    def writerecord(self, record : Dict[str, Any]) -> None:
        if self.csvwriter == None:
            self.init_csv(record)
        assert self.csvwriter
        self.csvwriter.writerow(self.filter_record(record))
        sys.stdout.flush()

    def init_csv(self, record : Dict[str, Any]) -> None:
        fieldnames = record.keys()
        self.csvkeys = self.filter_keys(fieldnames)
        fp = sys.stdout
        if self.output_filename:
            fp = open(self.output_filename, "w")
        self.csvwriter = csv.DictWriter(fp, self.csvkeys)
        self.csvwriter.writeheader()

    def filter_record(self, record: Dict[str, Any]) -> Dict[str, Any]:
        rv = {}
        for k, v in record.items():
            if k in self.csvkeys:
                rv[k] = v
        return rv
    
    def filter_keys(self, keys : Iterable[str]) -> List[str]:
        if self.details:
            return list(keys)
        rv = []
        for k in keys:
            if k in {"num", "timestamp", "pointcount", "frame_duration"}:
                rv.append(k)
            elif "age" in k:
                rv.append(k)
        return rv
    
    def statistics(self) -> None:
        pass        
         
def main():
    SetupStackDumper()
    assert __doc__ is not None
    parser = ArgumentParser(description=__doc__.strip())
    parser.add_argument("--details", action="store_true", help="Output detailed timing records")
    parser.add_argument("--savergb", metavar="N", type=int, default=0, help="Save RGB images for every N-th frame")
    parser.add_argument("--output", "-o", metavar="FILE", type=str, help="Store CSV output in FILE")

    args = parser.parse_args()
    
    beginOfRun(args)
    #
    # Create source
    #
    sourceFactory = activesource_factory_from_args(args)
    source = sourceFactory()
    source.request_metadata("timestamps")
    if args.savergb:
        source.request_metadata("rgb")

    kwargs = {}
    writer = DropWriter(args=args)
    sourceServer = SourceServer(source, writer, args)
    sourceThread = threading.Thread(target=sourceServer.run, args=(), name="cwipc_grab.SourceServer")
    writer.set_producer(sourceThread)

    #
    # Run everything
    #
    ok = False
    try:
        sourceThread.start()

        ok = writer.run()
            
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
    del sourceServer
    endOfRun(args)
    if not ok:
        sys.exit(1)
    
if __name__ == '__main__':
    main()
    
    
    
