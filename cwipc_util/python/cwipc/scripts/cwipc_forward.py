"""
Forward point cloud streams to cwipc_netserver or other server, or serve them directly.
"""
import sys
import os
import time
import threading
import time
import argparse
import traceback

from ..net import sink_netserver
from ..net import sink_encoder
from ..net import sink_passthrough
from ..net import sink_lldpkg
from ..net import sink_netingest
from ._scriptsupport import *

def main():
    SetupStackDumper()
    assert __doc__ is not None
    parser = ArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("--noforward", action="store_true", help="Don't forward pointclouds, only prints statistics at the end")
    
    output_selection_args = parser.add_argument_group("output selection").add_mutually_exclusive_group()
    output_selection_args.add_argument("--port", action="store", default=4303, type=int, metavar="PORT", help="Port to serve compressed pointclouds on (default: 4303)")
    output_selection_args.add_argument("--forward", action="store", metavar="HOST:PORT", help="Send compressed pointclouds to cwipc_netserver running on HOST, port PORT")
    output_selection_args.add_argument("--lldpkg", action="store", metavar="URL", help="Send compressed data to LLDashPackager URL in stead of serving. Example URL:  https://vrt-evanescent.viaccess-orca.com/pctest/")
    
    output_args = parser.add_argument_group("output arguments")
    output_args.add_argument("--seg_dur", action="store", type=int, metavar="MS", help="LLDashPackager segment duration (milliseconds, default 10000)")
    output_args.add_argument("--timeshift_buffer", action="store", type=int, metavar="MS", help="LLDashPackager timeshift buffer depth (milliseconds, default 30000)")
    output_args.add_argument("--noencode", action="store_true", help="Send uncompressed pointclouds (default: use cwipc_codec encoder)")
    output_args.add_argument("--octree_bits", action="append", type=int, metavar="N", help="Override/append encoder parameter (depth of octree)")
    output_args.add_argument("--jpeg_quality", action="append", type=int, metavar="N", help="Override/append encoder parameter (jpeg quality)")
    output_args.add_argument("--tiled", action="store_true", help="Encode and transmit streams for every tile")
    output_args.add_argument("--tile", action="append", type=int, help="Encode and transmit stream for specific tile, can be specified more than once.")
    args = parser.parse_args()
    beginOfRun(args)
    #
    # Create source
    #
    sourceFactory = activesource_factory_from_args(args)
    source = sourceFactory()
    #
    # Check how many streams we need
    #
    tiledescriptions = None
    if args.octree_bits or args.jpeg_quality or args.tiled:
        if args.tiled:
            assert hasattr(source, 'maxtile')
            tilecount = source.maxtile() # type: ignore
            td = [source.get_tileinfo_dict(i) for i in range(tilecount)] # type: ignore
            tiledescriptions = filter(lambda e: e['cameraMask'] != 0, td)
            tiledescriptions = list(tiledescriptions)
        elif args.tile:
            tiledescriptions = [source.get_tileinfo_dict(i) for i in args.tile] # type: ignore
        else:
            tiledescriptions = None
    n_tiles = len(tiledescriptions) if tiledescriptions else 1
    n_quality = len(args.jpeg_quality) if args.jpeg_quality and type(args.jpeg_quality) == list else 1
    n_octree_bits = len(args.octree_bits) if args.octree_bits and type(args.octree_bits) == list else 1
    n_streams = n_tiles * n_quality * n_octree_bits
    if args.verbose:
        print(f"cwipc_forward: {n_streams} streams")
    #
    # Find correct encoder factory
    #
    if args.noencode:
        encoder_factory = sink_passthrough.cwipc_sink_passthrough
    else:
        encoder_factory = sink_encoder.cwipc_sink_encoder
    if args.noforward:
        forwarder = None
    elif args.lldpkg:
        forwarder = encoder_factory(
            sink_lldpkg.cwipc_sink_lldpkg(
                args.lldpkg,
                seg_dur_in_ms=args.seg_dur,
                timeshift_buffer_depth_in_ms=args.timeshift_buffer, 
                verbose=(args.verbose > 1),
                nodrop=args.nodrop,
                nstream=n_streams
            ),
            verbose=(args.verbose > 1),
            nodrop=args.nodrop
        )
    elif args.forward:
        forwarder = encoder_factory(
            sink_netingest.cwipc_sink_netingest(
                args.forward,
                verbose=(args.verbose > 1),
                nodrop=args.nodrop,
                nstream=n_streams
            ),
            verbose=(args.verbose > 1),
            nodrop=args.nodrop
        )

    else:
        forwarder = encoder_factory(
            sink_netserver.cwipc_sink_netserver(
                args.port, 
                verbose=(args.verbose > 1),
                nodrop=args.nodrop,
                nstream=n_streams
            ),
            verbose=(args.verbose > 1),
            nodrop=args.nodrop
        )
    if args.octree_bits or args.jpeg_quality or tiledescriptions:
        forwarder.set_encoder_params(octree_bits=args.octree_bits, jpeg_quality=args.jpeg_quality, tiles=tiledescriptions) # type: ignore

    sourceServer = SourceServer(source, forwarder, args)
    sourceThread = threading.Thread(target=sourceServer.run, args=(), name="cwipc_forward.SourceServer")
    if forwarder:
        forwarder.set_producer(sourceThread)

    #
    # Run everything
    #
    try:
        sourceThread.start()

        if forwarder and hasattr(forwarder, 'start'):
            forwarder.start()
            
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
    if forwarder and hasattr(forwarder, 'stop'):
        forwarder.stop()
    if forwarder and hasattr(forwarder, 'statistics'):
        forwarder.statistics()
    sourceServer.statistics()
    del forwarder
    del sourceServer
    endOfRun(args)
    
if __name__ == '__main__':
    main()
    
    
    
