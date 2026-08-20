import sys
import os
import time
import signal
import argparse
import traceback
import warnings
from typing import cast, Union, List, Callable

from cwipc.abstract import cwipc_tileinfo_dict

from .. import cwipc_check_module,cwipc_pointcloud_wrapper, playback, cwipc_get_version, cwipc_proxy, cwipc_synthetic, cwipc_capturer, CWIPC_LOG_LEVEL_NONE, CWIPC_LOG_LEVEL_ERROR, CWIPC_LOG_LEVEL_WARNING, CWIPC_LOG_LEVEL_TRACE, CWIPC_LOG_LEVEL_DEBUG, cwipc_log_configure, cwipc_log_default_callback
from ..net import source_netclient
from ..net import source_decoder
from ..net import source_passthrough
from ..net import source_lldplay
from ..net import source_synchronizer
from ..net.abstract import *
from .. import filters

__all__ = [ 
    "SetupStackDumper",
    "BaseArgumentParser",
    "ArgumentParser",
    "activesource_factory_from_args",
    "SourceServer",
    "beginOfRun",
    "endOfRun"
]

def _dump_app_stacks(*args) -> None:
    """Print stack traces for all threads."""
    print(f"{sys.argv[0]}: QUIT received, dumping all stacks, {len(sys._current_frames())} threads:", file=sys.stderr)
    for threadId, stack in list(sys._current_frames().items()):
        print("\nThreadID:", threadId, file=sys.stderr)
        traceback.print_stack(stack, file=sys.stderr)
        print(file=sys.stderr)


def SetupStackDumper() -> None:
    """Install signal handler so `kill --QUIT` will dump all thread stacks, for debugging."""
    if hasattr(signal, 'SIGQUIT'):
        signal.signal(signal.SIGQUIT, _dump_app_stacks)

class _usused_pipeline_activesource(cwipc_activesource_abstract):
    """A wrapper around an active raw source feeding into a decoder.
    Produces the output of the decoder, but control commands are forwarded to the active reader.
    """
    def __init__(self, reader : cwipc_activerawsource_abstract, decoder : cwipc_source_abstract):
        self.__reader = reader
        self.__decoder = decoder

    def start(self) -> bool:
        return self.__reader.start()
    
    def stop(self) -> None:
        self.__reader.stop()

    def free(self) -> None:
        self.__decoder = None

    def eof(self) -> bool:
        if not self.__decoder:
            return True
        return self.__decoder.eof()

    def available(self, wait: bool) -> bool:
        if not self.__decoder:
            return False
        return self.__decoder.available(wait)

    def get(self) -> source_netclient.cwipc_pointcloud_abstract | None:
        if not self.__decoder:
            return None
        return self.__decoder.get() 

    def statistics(self) -> None:
        pass

    def reload_config(self, config: str | bytes | None) -> None:
        raise NotImplementedError

    def get_config(self) -> bytes:
        raise NotImplementedError

    def seek(self, timestamp: int) -> bool:
        raise NotImplementedError

    def request_metadata(self, name: str) -> None:
        raise NotImplementedError

    def is_metadata_requested(self, name: str) -> bool:
        raise NotImplementedError

    def auxiliary_operation(self, op: str, inbuf: bytes, outbuf: bytearray) -> bool:
        raise NotImplementedError

    def maxtile(self) -> int:
        raise NotImplementedError

    def get_tileinfo_dict(self, tilenum: int) -> dict[str, Any]:
        raise NotImplementedError


class pipelined_activesource_factory:
    def __init__(self, reader_factory : cwipc_activerawsource_factory_abstract, decoder_factory : cwipc_activedecoder_factory_abstract):
        self.reader_factory = reader_factory
        self.decoder_factory = decoder_factory

    def __call__(self) -> cwipc_activesource_abstract:
        reader = self.reader_factory()
        decoder = self.decoder_factory(reader)
        return decoder

def activesource_factory_from_args(args : argparse.Namespace, autoConfig : bool=False) -> cwipc_activesource_factory_abstract:
    """Create a cwipc_source based on command line arguments.
    Could be synthetic, realsense, kinect, proxy, ...
    Returns cwipc_source object and name commonly used in cameraconfig.xml.
    """
    source : cwipc_activesource_factory_abstract
    activedecoder_factory : Callable[[cwipc_activerawsource_abstract], cwipc_activesource_abstract]
    passivedecoder_factory : Callable[[cwipc_rawsource_abstract], cwipc_source_abstract]

    if args.nodecode:
        activedecoder_factory = source_passthrough.cwipc_activesource_passthrough
        passivedecoder_factory = source_passthrough.cwipc_source_passthrough
    else:
        activedecoder_factory = source_decoder.cwipc_activesource_decoder
        passivedecoder_factory = source_decoder.cwipc_source_decoder
    if args.kinect:
        if not cwipc_check_module("kinect"):
            print(f"{sys.argv[0]}: No support for Kinect grabber on this platform")
            sys.exit(-1)
        from .. import kinect
        if autoConfig:
            source = lambda config="auto": kinect.cwipc_kinect(config)
        elif args.cameraconfig:
            source = lambda config=args.cameraconfig: kinect.cwipc_kinect(config)
        else:
            source = lambda : kinect.cwipc_kinect()
    elif args.realsense:
        if not cwipc_check_module("realsense2"):
            print(f"{sys.argv[0]}: No support for realsense grabber on this platform")
            sys.exit(-1)
        from .. import realsense2
        if autoConfig:
            source = lambda config="auto": realsense2.cwipc_realsense2(config)
        elif args.cameraconfig:
            source = lambda config=args.cameraconfig: realsense2.cwipc_realsense2(config) # type: ignore
        else:
            source = lambda : realsense2.cwipc_realsense2()
    elif args.orbbec:
        if not cwipc_check_module("orbbec"):
            print(f"{sys.argv[0]}: No support for orbbec grabber on this platform")
            sys.exit(-1)
        from .. import orbbec
        if autoConfig:
            source = lambda config="auto": orbbec.cwipc_orbbec(config)
        elif args.cameraconfig:
            source = lambda config=args.cameraconfig: orbbec.cwipc_orbbec(config)
        else:
            source = lambda : orbbec.cwipc_orbbec()
    elif args.synthetic:
        source = lambda : cwipc_synthetic(fps=args.fps, npoints=args.npoints)
    elif args.proxy:
        source = lambda : cwipc_proxy('', args.proxy)
    elif args.playback:
        if not os.path.isdir(args.playback):
            filename = args.playback
            playback_type = _guess_playback_type([filename])
            if not playback_type:
                print(f'{sys.argv[0]}: {filename}: unknown playback file type')
                sys.exit(-1)
            source = lambda : playback.cwipc_playback([filename], ext=playback_type, fps=args.fps, loop=args.loop, inpoint=args.inpoint, outpoint=args.outpoint, retimestamp=args.retimestamp)
        else:
            dirname = args.playback
            # Could be a raw recording directory (with a cameraconfig.json) or a directory with pointclouds
            configfile = os.path.join(dirname, 'cameraconfig.json')
            if os.path.exists(configfile):
                _ = cwipc_check_module("kinect")
                _ = cwipc_check_module("orbbec")
                _ = cwipc_check_module("realsense2")

                source = lambda : cwipc_capturer(configfile)
            else:
                playback_type = _guess_playback_type(os.listdir(dirname))
                if not playback_type:
                    print(f'{sys.argv[0]}: {dirname}: should contain only one of .ply, .cwipcdump or .cwicpc files')
                    sys.exit(-1)
                source = lambda : playback.cwipc_playback(dirname, ext=playback_type, fps=args.fps, loop=args.loop, inpoint=args.inpoint, outpoint=args.outpoint, retimestamp=args.retimestamp)
    elif args.netclient:
        source = pipelined_activesource_factory(
            lambda : source_netclient.cwipc_source_netclient(args.netclient,verbose = (args.verbose > 1)),
            lambda rdr : activedecoder_factory(rdr, verbose = (args.verbose > 1))
        )
    elif args.mt_netclient:
        s_args = args.mt_netclient.split(':')
        n_tile = int(s_args[2])
        n_qual = int(s_args[3])
        netclient = f"{s_args[0]}:{s_args[1]}"
        def source_mt_netclient(netclient=netclient, n_tile=n_tile, n_qual=n_qual) -> cwipc_activesource_abstract:
            rdr = source_netclient.cwipc_multisource_netclient(
                netclient,
                n_tile,
                n_qual,
                verbose=(args.verbose > 1)
            )
            decoders : List[cwipc_source_abstract] = []
            for i in range(n_tile):
                dcdr = passivedecoder_factory(rdr.get_tile_source(i))
                decoders.append(dcdr)
            syncer = source_synchronizer.cwipc_source_synchronizer(rdr, decoders, verbose=(args.verbose > 1))
            return syncer
        source = source_mt_netclient

    elif args.lldplay:
        source = pipelined_activesource_factory(
            lambda : source_lldplay.cwipc_source_lldplay(args.lldplay, verbose = args.verbose > 1),
            lambda rdr : activedecoder_factory(rdr, verbose = (args.verbose > 1))
        )

    elif args.mt_lldplay:
        url = args.mt_lldplay
        n_tile = 0
        n_qual = 0
        def source_mt_lldplay(url=url, n_tile=n_tile, n_qual=n_qual) -> cwipc_activesource_abstract:
            rdr = source_lldplay.cwipc_multisource_lldplay(
                url,
                verbose = (args.verbose > 1)
            )
            n_tile = rdr.get_tile_count()
            decoders : List[cwipc_source_abstract] = []
            for i in range(n_tile):
                dcdr = passivedecoder_factory(rdr.get_tile_source(i))
                decoders.append(dcdr)
            syncer = source_synchronizer.cwipc_source_synchronizer(rdr, decoders, verbose=(args.verbose > 1))
            return syncer
        source = source_mt_lldplay
    else:
        # Default case: use the generic capturer.
        #
        # First we need to ensure all capturer DLLs are loaded (so they register themselves
        # with the generic capturer)
        #
        _ = cwipc_check_module("kinect")
        _ = cwipc_check_module("orbbec")
        _ = cwipc_check_module("realsense2")

        if autoConfig:
            source = lambda : cwipc_capturer("auto")
        elif args.cameraconfig:
            source = lambda : cwipc_capturer(args.cameraconfig)
        else:
            source = cast(cwipc_activesource_factory_abstract, cwipc_capturer)
    return source

def _guess_playback_type(filenames : List[str]) -> Optional[str]:
    has_ply = False
    has_dump = False
    has_compressed = False
    for fn in filenames:
        if fn.lower().endswith('.ply'): has_ply = True
        if fn.lower().endswith('.cwipcdump'): has_dump = True
        if fn.lower().endswith('.cwicpc'): has_compressed = True
    if int(has_ply)+int(has_dump)+int(has_compressed) != 1:
        return None     # Cop-out: if we don't have exactly one file type we don't know
    if has_ply:
        return '.ply'
    if has_dump:
        return '.cwipcdump'
    if has_compressed:
        return '.cwicpc'
    return None
    
class SourceServer:
    """Wrapper class around cwipc_grabber.
    run() will send pointclouds to viewer (or other consumer) through a feed() call.
    At the end statistics can be printed."""
    
    pc_filters : List[filters.cwipc_abstract_filter]

    def __init__(self, grabber : cwipc_activesource_abstract, viewer, args : argparse.Namespace, owns_grabber : bool = True):
        self.grabber = grabber
        self.verbose = args.verbose
        self.count = args.count
        self.inpoint = args.inpoint
        self.outpoint = args.outpoint
        self.cameraconfig = args.cameraconfig
        self.viewer = viewer
        self.times_grab : List[float] = []
        self.pointcounts_grab : List[int] = []
        self.latency_grab : List[float] = []
        self.lastGrabTime = None
        self.fps = None
        self.stopped = False
        self.pc_filters = []
        if args.filter:
            for fdesc in args.filter:
                filter = filters.factory(fdesc)
                self.pc_filters.append(filter)
        self.owns_grabber = owns_grabber
        if self.owns_grabber:
            ok = self.grabber.start()
            if not ok:
                print("grab: failed to start() grabber", flush=True)
                self.grabber = None
                self.stopped = True

    def __del__(self):
        self.stopped = True
        self.grabber = None

    def stop(self) -> None:
        if self.stopped: return
        if self.verbose: print("grab: stopping", flush=True)
        if self.grabber and self.owns_grabber:
            self.grabber.stop()
        self.stopped = True
        
    def grab_pc(self) -> Optional[cwipc_pointcloud_wrapper]:
        if self.lastGrabTime and self.fps:
            nextGrabTime = self.lastGrabTime + 1/self.fps
            if time.time() < nextGrabTime:
                time.sleep(nextGrabTime - time.time())
        if not self.grabber:
            return None
        if self.grabber.eof():
            return None
        if not self.grabber.available(True):
            if self.grabber.eof():
                return None
            print('grab: no pointcloud available')
            time.sleep(0.1)
            return None
        if not self.grabber:
            return None
        t_before = time.time()
        if self.verbose and self.lastGrabTime:
            print(f"grab: call get() after {t_before-self.lastGrabTime:.3f}")
        pc = self.grabber.get()
        self.lastGrabTime = time.time()
        if self.verbose:
            print(f"grab: get() took {self.lastGrabTime-t_before:.3f}")
        return cast(cwipc_pointcloud_wrapper, pc)
        
    def run(self) -> None:
        assert self.grabber
        if self.inpoint:
            result = self.grabber.seek(self.inpoint)
            if result:
                print(f'grab: seek to timestamp {self.inpoint} successful', flush=True)
            else:
                print(f'grab: Error: seek to timestamp {self.inpoint} failed', flush=True)
                sys.exit(-1)
        ok = True # xxxjack was: self.grabber.start()
        if not ok:
            print("grab: failed to start() grabber", flush=True)
            self.stopped = True
        else:
            if self.verbose: print('grab: started', flush=True)
        while not self.stopped and not self.grabber.eof():
            t0 = time.time()
            pc = self.grab_pc()
            if not pc:
                continue
            else:
                for filter in self.pc_filters:
                    pc = filter.filter(pc)
                self.pointcounts_grab.append(pc.count())
                pc_timestamp = pc.timestamp()/1000.0
                if self.verbose: print(f'grab: captured {pc.count()} points, ts={pc.timestamp()}')
                t1 = time.time()
                if self.viewer: 
                    t = pc.timestamp()
                    if self.inpoint and t<self.inpoint:
                        continue
                    if self.outpoint and t>self.outpoint:
                        self.count = 0
                        self.stop()
                        continue
                    self.viewer.feed(pc)
                else:
                    pc = None
                self.latency_grab.append(time.time()-pc_timestamp)
            self.times_grab.append(t1-t0)
            if self.count != None:
                self.count -= 1
                if self.count <= 0:
                    break
        if self.verbose: print('grab: stopped', flush=True)
    
    def statistics(self) -> None:
        self.print1stat('capture_duration', self.times_grab)
        self.print1stat('capture_pointcount', self.pointcounts_grab, isInt=True)
        self.print1stat('capture_latency', self.latency_grab)
        if self.grabber:
            self.grabber.statistics()
        for filter in self.pc_filters:
            filter.statistics()
        
    def print1stat(self, name : str, values : Union[List[int], List[float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print('grab: {}: count=0'.format(name))
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = 'grab: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = 'grab: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(name, count, avgValue, minValue, maxValue))

    def get_latencies(self) -> List[float]:
        return self.latency_grab
    
def BaseArgumentParser(*args, **kwargs) -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(*args, **kwargs)
    parser.add_argument("--version", action="store_true", help="Print version and exit")
    parser.add_argument("-v", "--verbose", action="count", default=0, help="Print information about each pointcloud while it is processed. Double for even more verbosity.")
    parser.add_argument("--logging", type=str, action="store", metavar="LEVEL", help="Set cwipc logging level (error, warning, info, debug) and capture log messages.")
    parser.add_argument("--pausefordebug", action="store_true", help="Pause at begin and end of run (to allow attaching debugger or profiler)")
    parser.add_argument("--debugpy", action="store_true", help="Pause at begin of run to wait for debugpy attaching")
    return parser

def ArgumentParser(*args, **kwargs) -> argparse.ArgumentParser:
    parser = BaseArgumentParser(*args, **kwargs)
    input_selection_args = parser.add_argument_group("input source selection").add_mutually_exclusive_group()
    parser.add_argument("--cameraconfig", action="store", help="Specify camera configuration file (default: ./cameraconfig.json). auto for any attached camera without configuration.")
    input_selection_args.add_argument("--realsense", action="store_true", help="Use Intel Realsense capturer (default: from camera configuration)")
    input_selection_args.add_argument("--kinect", action="store_true", help="Use Azure Kinect capturer (default: from camera configuration)")
    input_selection_args.add_argument("--orbbec", action="store_true", help="Use Orbbec capturer (default: from camera configuration)")
    input_selection_args.add_argument("--synthetic", action="store_true", help="Use synthetic pointcloud source")
    input_selection_args.add_argument("--proxy", type=int, action="store", metavar="PORT", help="Use proxyserver pointcloud source server, proxyserver listens on PORT")
    input_selection_args.add_argument("--netclient", action="store", metavar="HOST:PORT", help="Use (compressed) pointclouds from netclient, server runs on port PORT on HOST")
    input_selection_args.add_argument("--lldplay", action="store", metavar="URL", help="Use DASH (compressed) pointcloud stream from URL")
    input_selection_args.add_argument("--mt-netclient", action="store", metavar="HOST:PORT:NT:NQ", help="Use (compressed) pointclouds from netclient, server runs on port PORT on HOST. Multi-tile multi-quality with given number of tiles and qualities per tile.")
    input_selection_args.add_argument("--mt-lldplay", action="store", metavar="URL", help="Use DASH (compressed) pointcloud stream from URL. Multi-tile, multi-quality.")
    input_selection_args.add_argument("--playback", action="store", metavar="PATH", help="Use pointcloud(s) from ply or cwipcdump file or directory (in alphabetical order)")

    input_args = parser.add_argument_group("input arguments")
    input_args.add_argument("--nodecode", action="store_true", help="Receive uncompressed pointclouds with --netclient and --lldplay (default: compressed with cwipc_codec)")
    input_args.add_argument("--loop", action="store_true", help="With --playback loop the contents in stead of terminating after the last file")
    input_args.add_argument("--npoints", action="store", metavar="N", type=int, help="Limit number of points (approximately) in synthetic pointcoud", default=0)
    input_args.add_argument("--fps", action="store", type=int, help="Limit playback rate to FPS (for some capturers)", default=0)
    input_args.add_argument("--retimestamp", action="store_true", help="Set timestamps to wall clock in stead of recorded timestamps (for some grabbers)")
    input_args.add_argument("--count", type=int, action="store", metavar="N", help="Stop after receiving N pointclouds")
    input_args.add_argument("--inpoint", type=int, action="store", metavar="N", help="Start at frame with timestamp > N")
    input_args.add_argument("--outpoint", type=int, action="store", metavar="N", help="Stop at frame with timestamp >= N")
    input_args.add_argument("--nodrop", action="store_true", help="Attempt to store all captures by not dropping frames. Only works for some prerecorded capturers.")
    input_args.add_argument("--filter", action="append", metavar="FILTERDESC", help="After capture apply a filter to each point cloud. Multiple filters are applied in order.")
    input_args.add_argument("--help_filters", action="store_true", help="List available filters and exit")
    return parser
    
def waitForDebugpy() -> None:
    import debugpy  # type: ignore
    debugpy.listen(5678)
    print(f"{sys.argv[0]}: waiting for debugpy attach on 5678", flush=True)
    debugpy.wait_for_client()
    print(f"{sys.argv[0]}: debugger attached")

def beginOfRun(args : argparse.Namespace) -> None:
    """Optionally pause execution"""
    if args.version:
        print(cwipc_get_version())
        sys.exit(0)
    if args.help_filters:
        filters.help()
        sys.exit(0)
    if args.pausefordebug:
        answer=None
        while answer != 'Y':
            print(f"{sys.argv[0]}: starting, pid={os.getpid()}. Press Y to continue -", flush=True)
            answer = sys.stdin.readline()
            answer = answer.strip()
        print(f"{sys.argv[0]}: started.")
    if args.debugpy:
        waitForDebugpy()
    if args.logging:
        levelmap = {
            'error': CWIPC_LOG_LEVEL_ERROR,
            'warning': CWIPC_LOG_LEVEL_WARNING,
            'trace': CWIPC_LOG_LEVEL_TRACE,
            'debug': CWIPC_LOG_LEVEL_DEBUG
        }
        level = levelmap.get(args.logging.lower(), CWIPC_LOG_LEVEL_NONE)
        cwipc_log_configure(level, cwipc_log_default_callback)

def endOfRun(args : argparse.Namespace) -> None:
    """Optionally pause execution"""
    if args.pausefordebug:
        answer=None
        while answer != 'Y':
            print(f"{sys.argv[0]}: stopping, pid={os.getpid()}. Press Y to continue -", flush=True)
            answer = sys.stdin.readline()
            answer = answer.strip()
        print(f"{sys.argv[0]}: stopped.")
