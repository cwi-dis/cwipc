"""
View the synthetic example point cloud using a Qt window sink.
"""
import argparse
import cwipc

from cwipc.io.qtwindowsink import QtWindowSink


def main():
    # Parse arguments
    parser = argparse.ArgumentParser(description="Create synthetic pointclouds and show them in a Qt window sink.")
    parser.add_argument("-p", dest="npoints", type=int, help="Number of points.", default=0)
    parser.add_argument("-f", dest="fps", type=int, help="Frames per second.", default=0)
    args = parser.parse_args()

    # Create the synthetic generator
    generator = cwipc.cwipc_synthetic(args.fps, args.npoints)
    generator.start()
    assert generator.available(True), "Generator is not available"

    # Create the Qt window sink
    sink = QtWindowSink()

    # Stream point clouds from the generator and feed them to the sink
    while True:
        point_cloud = generator.get()
        assert point_cloud, "Generator does not have a point cloud."
        sink.feed(point_cloud)
        sink.process()
        if not sink.available:
            break


if __name__ == "__main__":
    main()
