"""
Copy a point cloud, a recorded point cloud stream or a raw RGBD camera recording. Can also convert, compress, etc.
"""
import sys
import os
import threading
import traceback
import queue
import struct
from typing import Optional, Dict, Any

from .. import cwipc_pointcloud_wrapper, cwipc_write, cwipc_write_debugdump, CwipcError, CWIPC_FLAGS_BINARY
from .. import codec
from ._scriptsupport import *
from ..net.abstract import *

class FileWriter(cwipc_sink_abstract):
    encoder : Optional[codec.cwipc_encoder_wrapper]
    producer : Optional[cwipc_producer_abstract]
    output_queue : queue.Queue[Optional[cwipc_pointcloud_wrapper]]

    def __init__(self, pcpattern : Optional[str]=None, rgbpattern : Optional[str]=None, depthpattern : Optional[str]=None, skeletonpattern : Optional[str]=None, verbose : bool=False, queuesize : int=2, nodrop : bool=False, flags : int=0):
        self.producer = None
        self.output_queue = queue.Queue(maxsize=queuesize)
        self.nodrop = nodrop
        self.verbose = verbose
        self.pcpattern = pcpattern
        self.rgbpattern = rgbpattern
        self.depthpattern = depthpattern
        self.skeletonpattern = skeletonpattern
        self.count = 0
        self.flags = flags
        self.error_encountered = False
        self.encoder = None
        
    def start(self) -> None:
        pass

    def stop(self) -> None:
        pass

    def setup_encoder(self, params : Dict[str, Any]):
        encoder_params = None
        if params:
            encoder_params = codec.cwipc_new_encoder_params()
            if 'help' in params:
                print('Default arguments for compression:')
                for k in dir(encoder_params):
                    if k[0] != '_':
                        v = getattr(encoder_params, k)
                        print(f'{k}={v}')
                sys.exit(1)
            for k in params:
                if not hasattr(encoder_params, k):
                    print(f'Unknown compression parameter {k}. help=1 for allowed parameters.')
                    sys.exit(1)
                setattr(encoder_params, k, params[k])
        self.encoder = codec.cwipc_new_encoder(params=encoder_params)
        
    def set_producer(self, producer : cwipc_producer_abstract):
        self.producer = producer    
        
    def run(self):
        while (self.producer and self.producer.is_alive()) or not self.output_queue.empty():
            try:
                pc = self.output_queue.get(timeout=0.5)
                assert pc
                self.count = self.count + 1
                ok = self.save_pc(pc)
                pc = None
                if not ok:
                    self.error_encountered = True
                    break
            except queue.Empty:
                pass
        if self.verbose:
            print(f"writer: stopped")
        return not self.error_encountered              
        
    def feed(self, pc : cwipc_pointcloud_wrapper) -> None:
        try:
            if self.nodrop:
                self.output_queue.put(pc)
            else:
                self.output_queue.put(pc, timeout=0.5)
            if self.verbose:
                print(f"writer: fed pointcloud {pc.timestamp()} to writer")
        except queue.Full:
            if self.verbose:
                print(f"writer: dropped pointcloud {pc.timestamp()}")

    def save_pc(self, pc : cwipc_pointcloud_wrapper) -> bool:
        """Save pointcloud"""
        if self.pcpattern:
            # Save pointcloud
            filename = self.pcpattern.format(timestamp=pc.timestamp(), count=self.count)
            ext = os.path.splitext(filename)[1].lower()
            if ext == '.ply':
                try:
                    cwipc_write(filename, pc, self.flags)
                    if self.verbose:
                        print(f"writer: wrote pointcloud to {filename}")
                except CwipcError as e:
                    print(f"writer: error: {e}")
                    self.error_encountered = True
            elif ext == '.cwipcdump':
                cwipc_write_debugdump(filename, pc)
                if self.verbose:
                    print(f"writer: wrote pointcloud to {filename}")
            elif ext == '.cwicpc':
                assert self.encoder
                self.encoder.feed(pc)
                cdata = self.encoder.get_bytes()
                with open(filename, 'wb') as fp:
                    fp.write(cdata)
            else:
                print(f"writer: Filetype unknown for pointcloud output: {filename}")
                return False
            # xxxjack could easily add compressed files, etc
        if self.rgbpattern or self.depthpattern or self.skeletonpattern:
            saved_any = False
            ok = self.save_images(pc)
            if ok:
                saved_any = True
            ok = self.save_skeletons(pc)
            if ok:
                saved_any = True
            if not saved_any:
                print(f"writer: did not find any requested metadata in pointcloud {pc.timestamp()}")
                #return False
        return True
    
    def save_images(self, pc : cwipc_pointcloud_wrapper) -> bool:
        if not self.rgbpattern and not self.depthpattern:
            return False
        metadata = pc.access_metadata()
        if metadata == None or metadata.count() == 0:
            return False
        import cv2
        anydone = False
        if self.rgbpattern:
            image_dict = metadata.get_all_images("rgb.")
            for serial, image in image_dict.items():
                name = "rgb." + serial
                filename = self.rgbpattern.format(timestamp=pc.timestamp(), count=self.count, type="rgb", name=name)
                ok = cv2.imwrite(filename, image)
                if ok:
                    anydone = True
        if self.depthpattern:
            image_dict = metadata.get_all_images("depth.")
            for serial, image in image_dict.items():
                name = "depth." + serial
                filename = self.depthpattern.format(timestamp=pc.timestamp(), count=self.count, type="depth", name=name)
                ok = cv2.imwrite(filename, image)
                if ok:
                    anydone = True

        return anydone
    
    def save_skeletons(self, pc : cwipc_pointcloud_wrapper) -> bool:
        if not self.skeletonpattern:
            return False
        metadata = pc.access_metadata()
        if metadata == None:
            return False
        anydone = False
        for i in range(metadata.count()):
            name = metadata.name(i)
            if not name.startswith('skeleton'):
                continue
            data = metadata.data(i)
            data_bytes = bytes(data)
            n_skeletons, n_joints = struct.unpack('II', data_bytes[:8])
            if n_skeletons > 0:
                filename = self.skeletonpattern.format(timestamp=pc.timestamp(), count=self.count, type=type, name=name)
                ext = os.path.splitext(filename)[1].lower()
                if ext != '.txt':
                    print(f"Couldn't save skeleton to {filename}. {ext} format not supported. Try txt")
                    continue
                with open(filename, "w") as f:
                    f.write('n_skeletons : '+str(n_skeletons)+'\n')
                    f.write('n_joints : '+str(n_joints)+'\n')
                    joint_byte_syze = 8 * 4 # 1 int and 7 floats = 8 elements of size 4 bytes
                    joints_size = joint_byte_syze * n_joints
                    total_size_joints = joints_size * n_skeletons
                    offset = 8
                    #print("Joints:")
                    for i in range(n_skeletons*n_joints):
                        joint_struct = struct.Struct('I 7f')
                        confidence, x, y, z, qw, qx, qy, qz = joint_struct.unpack_from(data,offset)
                        f.write(str((confidence, x, y, z, qw, qx, qy, qz))+'\n')
                        offset += joint_struct.size
                        #print((confidence, x, y, z, qw, qx, qy, qz))
                print(f"writer: wrote skeleton to {filename}")
                anydone = True
        return anydone

    def statistics(self) -> None:
        pass        
        
def main():
    SetupStackDumper()
    assert __doc__ is not None
    parser = BaseArgumentParser(description=__doc__.strip())
    parser.add_argument("--nopointclouds", action="store_true", help="For sequences, don't save pointclouds (but will save rgb, depth and skeletons if requested and available)")
    parser.add_argument("--cwipcdump", action="store_true", help="For sequences, save pointclouds as .cwipcdump (default: .ply or dependent on output filename)")
    parser.add_argument("--compress", action="store_true", help="For sequences, save pointclouds as compressed .cwicpc (default: .ply or dependent on output filename)")
    parser.add_argument("--compress_param", action="append", metavar="NAME=VALUE", help="Add compressor parameter (help=1 for help)")
    parser.add_argument("--binary", action="store_true", help="Save pointclouds as binary .ply (default: ASCII .ply)")
    parser.add_argument("--rgb", action="store", metavar="EXT", help="For sequences, save captured RGB as images of type EXT")
    parser.add_argument("--depth", action="store", metavar="EXT", help="For sequences, save captured depth as images of type EXT")
    parser.add_argument("--skeleton", action="store", metavar="EXT", help="For sequences, save captured skeleton metadata as files of type EXT")
    parser.add_argument("--fpattern", action="store", metavar="VAR", default="count:04d", help="For sequences, construct filenames using VAR, which can be count or timestamp (default)")
    parser.add_argument("--filter", action="append", metavar="FILTERDESC", help="After capture apply a filter to each point cloud. Multiple filters are applied in order.")
    parser.add_argument("--help_filters", action="store_true", help="List available filters and exit")
    parser.add_argument("input", help="Point cloud (ply, cwipcdump, cwicpc) or directory of those, or cameraconfig.json file")
    parser.add_argument("output", action="store", help="Output filename (ply, cwipcdump, cwicpc) for single inputs, output directory for streams")

    args = parser.parse_args()
    stream_input = True
    stream_output = True
    if args.input.endswith(".json"):
        args.cameraconfig = args.input
        args.playback = None
        stream_input = True
    elif os.path.isdir(args.input):
        args.playback = args.input
        args.cameraconfig = None
        stream_input = True
    else:
        args.playback = args.input
        args.cameraconfig = None
        stream_input = False
    if args.output.endswith(".ply"):
        stream_output = False
        args.cwipcdump = False
        args.compress = False
    elif args.output.endswith(".cwipcdump"):
        stream_output = False
        args.cwipcdump = True
        args.compress = False
    elif args.output.endswith(".cwicpc"):
        stream_output = False
        args.cwipcdump = False
        args.compress = True
    elif os.path.isdir(args.output):
        stream_output = True
    elif os.path.exists(args.output):
        print(f"{parser.prog}: unknown extension in output file: {args.output}", file=sys.stderr)
        return 2
    else:
        os.mkdir(args.output)
        stream_output = True
    if stream_input and not stream_output:
        print(f"{parser.prog}: Cannot convert stream input to single point cloud output file", file=sys.stderr)
        return 1
    if not stream_input and stream_output:
        print(f"{parser.prog}: Cannot convert single point cloud input to stream output", file=sys.stderr)
        return 1
    if not stream_input and (args.rgb or args.nopointclouds or args.depth or args.skeleton):
        print(f"{parser.prog}: Arguments --rgb, --nopointclouds, --depth or --skeleton only make sense for streams")
        return 1
    # Fill argument values that we don't use but are needed by some of the modules.
    args.netclient = None
    args.lldplay = None
    args.endpaused = True
    args.loop = False
    args.nodecode = False
    args.kinect = None
    args.orbbec = None
    args.realsense = None
    args.synthetic = None
    args.proxy = None
    args.fps = None
    args.inpoint = None # xxxjack might be useful
    args.outpoint = None # xxxjack might be useful
    args.retimestamp = None # xxxjack might be useful
    args.nodrop = True
    args.count = None    
    beginOfRun(args)
    #
    # Create source
    #
    sourceFactory = activesource_factory_from_args(args)
    source = sourceFactory()
    #
    # Determine which output formats we want, set output filename pattern
    # and ensure the requested data is included by the capturer
    #
    if stream_output:
        if args.nopointclouds:
            pcpattern = None
        elif args.cwipcdump:
            pcpattern = f"{args.output}/pointcloud-{{{args.fpattern}}}.cwipcdump"
        elif args.compress:
            pcpattern = f"{args.output}/pointcloud-{{{args.fpattern}}}.cwicpc"
        else:
            pcpattern = f"{args.output}/pointcloud-{{{args.fpattern}}}.ply"
        rgbpattern = None
        if args.rgb:
            rgbpattern = f"{args.output}/{{name}}-{{{args.fpattern}}}.{args.rgb}"
            source.request_metadata("rgb")
        depthpattern = None
        if args.depth:
            depthpattern = f"{args.output}/{{name}}-{{{args.fpattern}}}.{args.depth}"
            source.request_metadata("depth")
        skeletonpattern = None
        if args.skeleton:
            skeletonpattern = f"{args.output}/{{name}}-{{{args.fpattern}}}.{args.skeleton}"
            source.request_metadata("skeleton")
    else:
        pcpattern = args.output
        rgbpattern = None
        depthpattern = None
        skeletonpattern = None
    kwargs = {}
    if args.nodrop:
        kwargs['nodrop'] = True
    if args.binary:
        kwargs['flags'] = CWIPC_FLAGS_BINARY
    writer = FileWriter(
        pcpattern=pcpattern,
        rgbpattern=rgbpattern,
        depthpattern=depthpattern,
        skeletonpattern=skeletonpattern,
        verbose=args.verbose,
        **kwargs
        )
    if args.compress:
        params = {}
        for sparam in args.compress_param or []:
            k, v = sparam.split('=')
            params[k] = eval(v)
        writer.setup_encoder(params)
    sourceServer = SourceServer(source, writer, args)
    sourceThread = threading.Thread(target=sourceServer.run, args=(), name="cwipc_copy.SourceServer")
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
        return 1
    return 0
    
if __name__ == '__main__':
    sts = main()
    sys.exit(sts)
    
    
    
