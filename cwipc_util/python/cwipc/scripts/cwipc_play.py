"""
View a point cloud, a recorded point cloud stream or a raw RGBD camera recording.
"""
import sys
import threading
import argparse
import traceback
from ._scriptsupport import *
from ..net.abstract import *
from ..io.visualizer import Visualizer

def help_commands():
    print(Visualizer.HELP)

def main():
    SetupStackDumper()
    assert __doc__ is not None
    parser = BaseArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--paused", action="store_true", help="Start paused (not needed for single point clouds)")
    parser.add_argument("--filter", action="append", metavar="FILTERDESC", help="After capture apply a filter to each point cloud. Multiple filters are applied in order.")
    parser.add_argument("--help_filters", action="store_true", help="List available filters and exit")
    parser.add_argument("--help_commands", action="store_true", help="List interactive commands and exit")
    parser.add_argument("input", help="Point cloud (ply, cwipcdump) or directory of those, or raw recording directory or cameraconfig.json")
    args = parser.parse_args()
    if args.help_commands:
        help_commands()
        sys.exit(0)
    args.timestamps = False
    if args.input.endswith(".json"):
        args.cameraconfig = args.input
        args.playback = None
    else:
        args.playback = args.input
        args.cameraconfig = None
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
    args.inpoint = None
    args.outpoint = None
    args.retimestamp = None
    args.nodrop = True
    args.rgb = None
    args.rgb_full = None
    args.rgb_cw = None
    args.rgb_ccw = None
    args.count = None
    # xxxjack or pause-at-end?
    beginOfRun(args)
    #
    # Create source
    #
    sourceFactory = activesource_factory_from_args(args)
    source = sourceFactory()
    visualizer = Visualizer(args=args)

    sourceServer = SourceServer(source, visualizer, args)
    sourceThread = threading.Thread(target=sourceServer.run, args=(), name="cwipc_play.SourceServer")
    if visualizer:
        visualizer.set_producer(sourceThread)
        visualizer.set_source(source)

    #
    # Run everything
    #
    try:
        sourceThread.start()

        if visualizer:
            visualizer.run()
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
    del visualizer
    del sourceServer
    endOfRun(args)
    
if __name__ == '__main__':
    main()
    
    
    
