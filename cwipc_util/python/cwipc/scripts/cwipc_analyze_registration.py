"""
Find registration of a tiled point cloud.
"""
import sys
from typing import Optional, List, Tuple
import argparse
import traceback
import cwipc
from cwipc.registration.abstract import AnalysisResults
from cwipc.registration.fine import RegistrationComputer_ICP_Point2Point
import cwipc.registration.analyze
from cwipc.registration.util import transformation_topython, get_tiles_used, cwipc_compute_tile_occupancy
from cwipc.registration.plot import Plotter

class AnalyzePointCloud:
    def __init__(self, args : argparse.Namespace):
        self.args = args
        self.verbose = args.verbose
        self.source_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.target_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.result_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        if self.args.algorithm_analyzer:
            self.analyzer_class = getattr(cwipc.registration.analyze, args.algorithm_analyzer)
        else:
            self.analyzer_class = cwipc.registration.analyze.DEFAULT_ANALYZER_ALGORITHM
            
    def load_source(self, source: str):
        pc = cwipc.cwipc_read(source, 0)
        self.source_pc = pc

    def load_target(self, target: str):
        pc = cwipc.cwipc_read(target, 0)
        self.target_pc = pc
    
    def run(self):
        assert self.source_pc
        if self.target_pc == None:
            self.target_pc = self.source_pc
        tiles = get_tiles_used(self.source_pc)
        if (not tiles or len(tiles) <= 1) and not (self.args.toself or self.args.togroundtruth):
            print(f"Source point cloud {self.source_pc} has no tiles or only one tile, cannot analyze registration.")
            return
        print(f"Tiles used in source: {tiles}")
        allResults = []
        todo : List[Tuple[int, int]] = []
        if self.args.togroundtruth:
            title = "Distance between this tile and ground-truth"
            for tile in tiles:
                todo.append([tile, 0])
        elif self.args.toself:
            title = "Distance between adjacent points in the same tile"
            for tile in tiles:
                todo.append((tile, tile))
        elif self.args.totile >= 0:
            title = f"Distance between this tile and tile {self.args.totile}"
            for sourcetile in tiles:
                if sourcetile != self.args.totile:
                    todo.append((sourcetile, self.args.totile))
        elif self.args.pairwise:
            title = "Distance between each pair of tiles"
            for sourcetile in tiles:
                for targettile in tiles:
                    if sourcetile != targettile:
                        todo.append((sourcetile, targettile))
        else:
            title = "Distance between each tile and all other tiles combined"
            for sourcetile in tiles:
                targettile = 255 - sourcetile
                todo.append((sourcetile, targettile))
        for sourcetile, targettile in todo:
            results = self.analyze_pointclouds(self.source_pc, sourcetile, self.target_pc, targettile)
            allResults.append(results)
        # xxxjack better not to sort allResults.sort(key=lambda r: (r.minCorrespondence))
        if self.args.measure:
            title += f"\n(correspondence: {self.args.measure[0]})"
        if self.args.plot:
            plotter = Plotter(title=title)
            plotter.set_results(allResults)
            plotter.plot(show=True)
        if self.args.occupancy >= 0:
            tiles_and_counts = cwipc_compute_tile_occupancy(self.source_pc, cellsize=self.args.occupancy, filterfloor=self.args.ignore_floor)
            for tilenum, count in tiles_and_counts:
                print(f"Occupancy: tilenum={tilenum}, count={count}, ncamera={tilenum.bit_count()}")
 
    def analyze_pointclouds(self, source: cwipc.cwipc_pointcloud_wrapper, sourcetile : int, target : cwipc.cwipc_pointcloud_wrapper, targettile : int) -> AnalysisResults:
        analyzer = self.analyzer_class()
        if self.args.toself:
            analyzer.set_ignore_nearest(self.args.nth)
        if self.args.max_corr >= 0:
            analyzer.set_max_correspondence_distance(self.args.max_corr)
        if self.args.min_corr > 0:
            analyzer.set_min_correspondence_distance(self.args.min_corr)
        if self.args.measure:
            analyzer.set_correspondence_measure(*self.args.measure)
        if self.args.ignore_floor:
            analyzer.set_ignore_floor(True)
        analyzer.verbose = self.verbose
        analyzer.set_source_pointcloud(source, sourcetile)
        analyzer.set_reference_pointcloud(target, targettile)
        analyzer.run()
        results = analyzer.get_results()
        correspondence = results.minCorrespondence
        if self.args.toself:
            label = f"{sourcetile:#x} self, nth={self.args.nth}"
        else:
            label = f"{sourcetile:#x} to {targettile:#x}"
        print(f"Alignment {label}: {results.tostr()}")
        if self.args.overlap:
            overlap_analyzer = cwipc.registration.analyze.OverlapAnalyzer()
            overlap_analyzer.verbose = self.verbose
            # NOTE: we are reversing the roles of source and target here, because we want to compute the overlap of the source tile with the target tile
            overlap_analyzer.set_reference_pointcloud(target, sourcetile)
            overlap_analyzer.set_source_pointcloud(source, targettile)
            overlap_analyzer.set_correspondence(correspondence)
            overlap_analyzer.run()
            overlap_results = overlap_analyzer.get_results()
            print(f"Alignment {label}: overlap fitness: {overlap_results.fitness:.6f}, inlier rmse: {overlap_results.rmse:.6f}")
        return results

def main():
    assert __doc__ is not None
    parser = argparse.ArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("source", help="Point cloud, as .ply or .cwipc file")
    parser.add_argument("--plot", action="store_true", help="Plot analysis distance distribution")
    parser.add_argument("--pairwise", action="store_true", help="Analyze pairwise registration of all tilecombinations, not just tile to all other tiles")
    parser.add_argument("--toself", action="store_true", help="Analyze self-registration (source and target tile the same), for judging capture quality")
    parser.add_argument("--totile", type=int, metavar="NUM", default=-1, help="Analyze registration of source tile NUM to each individual other tiles")
    parser.add_argument("--togroundtruth", type=str, metavar="PLYFILE", help="Analyze registration of every tile in the source to a ground truth point cloud TARGET")
    parser.add_argument("--nth", type=int, default=1, metavar="NTH", help="For --toself, use the NTH closest point to each point in the same tile (default: 1, i.e. nearest point)")
    parser.add_argument("--max_corr", type=float, default=-1, metavar="DIST", help="Maximum distance between two points to be considered the same point (default: -1, i.e. no limit)")
    parser.add_argument("--min_corr", type=float, default=0.0, metavar="DIST", help="Minimum distance between two points that is meaningful to consider (default: 0.0, i.e. no limit)")
    parser.add_argument("--measure", action="append", type=str, default=None, metavar="METHOD", help="Method to use for correspondence: mean, median, mode or 2mode (default: mean) ")
    parser.add_argument("--ignore_floor", action="store_true", help="Remove floor (low Y points)")
    parser.add_argument("--overlap", action="store_true", help="Also compute overlap between source and target tile")
    parser.add_argument("--occupancy", action="store", type=float, metavar="DIST", default=-1, help="Also compute source tile occupancy (after downsampling to DIST if DIST > 0)")
    parser.add_argument("--algorithm_analyzer", action="store", help="Analyzer algorithm to use")
    parser.add_argument("--help_algorithms", action="store_true", help="Show available algorithms and a short description of them")
    parser.add_argument("--verbose", action="store_true", help="Verbose output")
    parser.add_argument("--debugpy", action="store_true", help="Wait for debugpy client to attach")
    args = parser.parse_args()
    if args.debugpy:
        import debugpy
        debugpy.listen(5678)
        print(f"{sys.argv[0]}: waiting for debugpy attach on 5678", flush=True)
        debugpy.wait_for_client()
        print(f"{sys.argv[0]}: debugger attached")
    if args.help_algorithms:
        print(cwipc.registration.analyze.HELP_ANALYZER_ALGORITHMS)
        return 0
    finder = AnalyzePointCloud(args)
    if args.togroundtruth:
        finder.load_target(args.togroundtruth)
    finder.load_source(args.source)
    finder.run()
    
if __name__ == '__main__':
    main()
    
    
    
