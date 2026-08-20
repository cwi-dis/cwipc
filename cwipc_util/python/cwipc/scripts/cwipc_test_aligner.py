"""
Find transform between two point clouds and apply it to align them.
"""
import sys
from typing import Optional, List
import argparse
import traceback
import cwipc
import cwipc.registration
import cwipc.registration.multicamera
import cwipc.registration.fine
import cwipc.registration.analyze
from cwipc.registration.abstract import RegistrationTransformation
from cwipc.registration.util import transformation_topython, cwipc_tilefilter_masked
from cwipc.registration.plot import Plotter
from cwipc.registration.cameraconfig import CameraConfig

class AlignmentFinder:
    def __init__(self, args : argparse.Namespace):
        self.args = args
        self.plot = args.plot
        self.verbose = args.verbose
        self.correspondence = args.correspondence
        self.input_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.result_pc : Optional[cwipc.cwipc_pointcloud_wrapper] = None
        self.transformations : List[RegistrationTransformation] = []
        self.cameraconfig : Optional[CameraConfig] = None
        if self.args.cameraconfig:
            self.cameraconfig = CameraConfig(self.args.cameraconfig)
            self.cameraconfig.load_from_file()

        if self.args.togroundtruth:
            self.multicamera_aligner_class = cwipc.registration.multicamera.MultiCameraToGroundTruth
        elif self.args.algorithm_multicamera:
            self.multicamera_aligner_class = getattr(cwipc.registration.multicamera, args.algorithm_multicamera)
        else:
            self.multicamera_aligner_class = cwipc.registration.multicamera.DEFAULT_MULTICAMERA_ALGORITHM

        if self.args.algorithm_fine:
            self.alignment_class = getattr(cwipc.registration.fine, args.algorithm_fine)
        else:
            self.alignment_class = None # The fine alignment class is determined by the multicamera aligner chosen.

        if self.args.algorithm_analyzer:
            self.analyzer_class = getattr(cwipc.registration.analyze, args.algorithm_analyzer)
        else:
            self.analyzer_class = cwipc.registration.analyze.DEFAULT_ANALYZER_ALGORITHM

        self.multi_aligner = self.multicamera_aligner_class()
        self.multi_aligner.verbose = self.verbose
        if self.correspondence:
            self.multi_aligner.set_max_correspondence(self.correspondence)
        if self.args.togroundtruth:
            pc = cwipc.cwipc_read(self.args.togroundtruth, 0)
            self.multi_aligner.set_groundtruth(pc)
        self.multi_aligner.set_analyzer_class(self.analyzer_class)
        if self.alignment_class is not None:
            self.multi_aligner.set_aligner_class(self.alignment_class)
        self.multi_aligner.show_plot = self.plot
            
    def load_input(self, source: str):
        pc = cwipc.cwipc_read(source, 0)
        self.input_pc = pc
    
    def save_output(self, filename: str):
        assert self.output_pc
        cwipc.cwipc_write(filename, self.output_pc)
        
    def run(self):
        assert self.input_pc
        self.multi_aligner.set_tiled_pointcloud(self.input_pc)
        if self.cameraconfig:
            for cam_index in range(self.cameraconfig.camera_count()):
                self.multi_aligner.set_original_transform(cam_index, self.cameraconfig.get_transform(cam_index).get_matrix())
        self.multi_aligner.run()
        self.output_pc = self.multi_aligner.get_result_pointcloud_full()
        self.transformations = self.multi_aligner.get_result_transformations()
        for i in range(len(self.transformations)):
            print(f"Tile {i} transformation:\n {self.transformations[i]}")
            if self.cameraconfig:
                matrix = self.transformations[i]
                t = self.cameraconfig.get_transform(i)
                t.set_matrix(matrix)
        if self.cameraconfig and self.cameraconfig.is_dirty():
            self.cameraconfig.save()
    
def main():
    assert __doc__ is not None
    parser = argparse.ArgumentParser(description=__doc__.strip(), formatter_class=argparse.RawDescriptionHelpFormatter)
    parser.add_argument("input", help="Point cloud, as .ply or .cwipc file")
    parser.add_argument("output", help="Point cloud, as .ply or .cwipc file")
    parser.add_argument("--plot", action="store_true", help="Plot analysis distance distribution")
    parser.add_argument("--algorithm_analyzer", action="store", help="Analyzer algorithm to use")
    parser.add_argument("--algorithm_multicamera", action="store", help="Fine alignment outer algorithm to use, for multiple cameras")
    parser.add_argument("--algorithm_fine", action="store", help="Fine alignment inner registration algorithm to use")
    parser.add_argument("--togroundtruth", metavar="PC", help="Use multicamera algorithm MultiCameraToGroundTruth with ground truth PC")
    parser.add_argument("--help_algorithms", action="store_true", help="Show available algorithms and a short description of them")
    parser.add_argument("--cameraconfig", metavar="CCFILE", help="Apply resulting transformations to cameraconfig.json file")
    parser.add_argument("--correspondence", type=float, metavar="D", help="Maximum correspondence distance for alignment (default: use analysis result)")
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
        print(cwipc.registration.fine.HELP_FINE_ALIGNMENT_ALGORITHMS)
        print(cwipc.registration.multicamera.HELP_MULTICAMERA_ALGORITHMS)
        return 0
    finder = AlignmentFinder(args)
    finder.load_input(args.input)
    finder.run()
    finder.save_output(args.output)
    
if __name__ == '__main__':
    main()
    
    
    
