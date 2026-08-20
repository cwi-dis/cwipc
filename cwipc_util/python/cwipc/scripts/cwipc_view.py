"""
View live point cloud streams from cameras, network sources or recordings.
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
    parser = ArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--nodisplay", action="store_true", help="Don't display pointclouds, only prints statistics at the end")
    parser.add_argument("--paused", action="store_true", help="Start paused")
    parser.add_argument("--skeleton", action="store_true", help="Get and render skeleton from the capture (in stead of point cloud). Only for capturers that support skeleton capture")
    parser.add_argument("--rgb", action="store_true", help="Show RGB captures in addition to point clouds")
    parser.add_argument("--rgb_cw", action="store_true", help="When showing RGB captures first rotate the 90 degrees clockwise")
    parser.add_argument("--rgb_ccw", action="store_true", help="When showing RGB captures first rotate the 90 degrees counterclockwise")
    parser.add_argument("--rgb_full", action="store_true", help="When showing RGB captures don't scale and combine but show every image in its own window")
    parser.add_argument("--timestamps", action="store_true", help="Print detailed timestamp information about every point cloud displayed")
    parser.add_argument("--latencies", action="store", metavar="FILE", help="Output grab latencies to FILE (or - for stdout)")
    parser.add_argument("--help_commands", action="store_true", help="List interactive commands and exit")
    args = parser.parse_args()
    if args.help_commands:
        help_commands()
        sys.exit(0)
    beginOfRun(args)
    #
    # Create source
    #
    sourceFactory = activesource_factory_from_args(args)
    source = sourceFactory()
    if not args.nodisplay:
        visualizer = Visualizer(args=args)
    else:
        visualizer = None

    if args.timestamps:
        source.request_metadata("timestamps")
    if args.skeleton:
        source.request_metadata("skeleton")
    if args.rgb:
        source.request_metadata("rgb")

    sourceServer = SourceServer(source, visualizer, args)
    sourceThread = threading.Thread(target=sourceServer.run, args=(), name="cwipc_view.SourceServer")
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
    if args.latencies:
        latency_values = sourceServer.get_latencies()
        if args.latencies == "-":
            ofp = sys.stdout
            print(f"latencies ({len(latency_values)} values):")
        else:
            ofp = open(args.latencies)
        for v in latency_values:
            print(f"{v:.3f}", file=ofp)

    del visualizer
    del sourceServer
    endOfRun(args)
    
if __name__ == '__main__':
    main()
    
    
    
