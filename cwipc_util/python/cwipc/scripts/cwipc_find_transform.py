"""
Find transform between two point clouds.
"""
import sys
from typing import Optional
import argparse
import traceback
import cwipc
from cwipc.registration.fine import RegistrationComputer_ICP_Point2Plane
from cwipc.registration.analyze import RegistrationAnalyzer, AnalysisResults
from cwipc.registration.util import transformation_topython, cwipc_tilefilter_masked
from cwipc.registration.plot import Plotter

class TransformFinder:
    def __init__(self, args : argparse.Namespace):
        self.args = args
        self.plot = args.plot
        self.dump = args.dump
        self.verbose = args.verbose
        self.correspondence = args.correspondence
        self.source_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.source_tile = args.sourcetile
        self.target_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.target_tile = args.targettile
        self.result_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.aligner = RegistrationComputer_ICP_Point2Plane()
        self.aligner.verbose = self.verbose
            

    def load_source(self, source: str):
        pc = cwipc.cwipc_read(source, 0)
        if self.source_tile:
            pc = cwipc_tilefilter_masked(pc, self.source_tile)
        self.source_pc = pc

    def load_target(self, target: str):
        pc = cwipc.cwipc_read(target, 0)
        if self.target_tile:
            pc = cwipc_tilefilter_masked(pc, self.target_tile)
        self.target_pc = pc

    def save_output(self, filename: str):
            assert self.result_pc
            cwipc.cwipc_write(filename, self.result_pc)
            
    def _fnmod(self) -> str:
        if self.source_tile or self.target_tile:
            return f"_{self.source_tile}_{self.target_tile}"
        return ""
    
    def run(self):
        assert self.source_pc
        assert self.target_pc
        if self.dump:
            self.dump_pointclouds(f"find_transform_before{self._fnmod()}.ply", self.source_pc, self.target_pc)
        analysis_results = self.analyze_pointclouds("Before", self.source_pc, self.target_pc)
        if self.correspondence < 0:
            self.correspondence = analysis_results.minCorrespondence
        print(f"Using aligner {self.aligner.__class__.__name__} with correspondence threshold {self.correspondence}")
        self.aligner.set_reference_pointcloud(self.target_pc)
        self.aligner.set_source_pointcloud(self.source_pc)
        self.aligner.set_correspondence(self.correspondence)
        self.aligner.run()
        transform = self.aligner.get_result_transformation()
        self.result_pc = self.aligner.get_result_pointcloud()
        if self.dump:
            cwipc.cwipc_write(f"find_transform_result{self._fnmod()}.ply", self.result_pc)
            self.dump_pointclouds(f"find_transform_after{self._fnmod()}.ply", self.result_pc, self.target_pc)
        self.analyze_pointclouds("After", self.result_pc, self.target_pc)
        p_transform = transformation_topython(transform)
        print(f"Transform filter needed: --filter 'transform44({p_transform})'")

    def dump_pointclouds(self, filename: str, source: cwipc.cwipc_pointcloud_wrapper, target: cwipc.cwipc_pointcloud_wrapper):
        if self.verbose:
            print(f"Dumping point clouds to {filename}")
        colored_source = cwipc.cwipc_colormap(source, 0xFFFFFFFF, 0xAAFF0000)
        colored_target = cwipc.cwipc_colormap(target, 0xFFFFFFFF, 0xAA00FF00)
        combined = cwipc.cwipc_join(colored_source, colored_target)
        cwipc.cwipc_write(filename, combined)
        
    def analyze_pointclouds(self, label : str, source: cwipc.cwipc_pointcloud_wrapper, target: cwipc.cwipc_pointcloud_wrapper) -> AnalysisResults:
        analyzer = RegistrationAnalyzer()
        analyzer.verbose = self.verbose
        analyzer.set_reference_pointcloud(target)
        analyzer.set_source_pointcloud(source)
        if self.args.measure:
            analyzer.set_correspondence_measure(*self.args.measure)
        analyzer.run()
        results = analyzer.get_results()
        print(f"{label} alignment: {results.tostr()}")
        if self.plot:
            plotter = Plotter(title=label)
            plotter.set_results([results])
            plotter.plot(show=True)
        return results
    
def main():
    assert __doc__ is not None
    parser = argparse.ArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", help="Point cloud, as .ply or .cwipc file")
    parser.add_argument("target", help="Point cloud, as .ply or .cwipc file")
    parser.add_argument("--plot", action="store_true", help="Plot analysis distance distribution")
    parser.add_argument("--dump", help="Dump combined pre and post point clouds to files (color-coded)", action="store_true")
    parser.add_argument("--measure", action="append", type=str, default=None, metavar="METHOD", help="Method to use for correspondence analysis: mean, median, or mode (default: mean). Specify multiple to see more statistics.")
    parser.add_argument("--sourcetile", type=int, metavar="NUM", default=0, help="Filter source point cloud to tile NUM before alignment")
    parser.add_argument("--targettile", type=int, metavar="NUM", default=0, help="Filter target point cloud to tile NUM before alignment")
    parser.add_argument("--correspondence", type=float, metavar="FLOAT", default=-1, help="Correspondence threshold for alignment (default: use analysis result)")
    parser.add_argument("--output", help="Output point cloud, as .ply or .cwipc file. NOTE: this is a single tile if --sourcetile is specified.")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--debugpy", action="store_true", help="Wait for debugpy client to attach")
    args = parser.parse_args()
    if args.debugpy:
        import debugpy
        debugpy.listen(5678)
        print(f"{sys.argv[0]}: waiting for debugpy attach on 5678", flush=True)
        debugpy.wait_for_client()
        print(f"{sys.argv[0]}: debugger attached")
    finder = TransformFinder(args)
    finder.load_source(args.source)
    finder.load_target(args.target)
    finder.run()
    if args.output:
        finder.save_output(args.output)
    
if __name__ == '__main__':
    main()
    
    
    
