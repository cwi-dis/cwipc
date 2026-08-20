import sys
from abc import ABC, abstractmethod
import copy
import math
import functools
from typing import List, Optional, Any, Tuple
try:
    from typing import override
except ImportError:
    from typing_extensions import override
import numpy as np
import numpy.linalg
import scipy.spatial
from matplotlib import pyplot as plt
from cwipc import cwipc_pointcloud_wrapper, cwipc_from_numpy_matrix, cwipc_join, CwipcError

from cwipc.registration.abstract import RegistrationTransformation
from .. import cwipc_pointcloud_wrapper, cwipc_tilefilter, cwipc_downsample, cwipc_write, cwipc_colormap
from .abstract import *
from .util import *
from .fine import RegistrationComputer_ICP_Point2Plane, DEFAULT_FINE_ALIGNMENT_ALGORITHM
from .analyze import RegistrationAnalyzer
from .plot import Plotter

OrderedCameraList = List[Tuple[int, int, float, float]] # Cameranumber, tilemask, correspondence, belowcorrespondencefraction
class BaseMulticamAlignmentAlgorithm(MulticamAlignmentAlgorithm, BaseMulticamAlgorithm):
    """\
    Base class for multi-camera alignment algorithms.
    """
    original_pointcloud : Optional[cwipc_pointcloud_wrapper]
    transformations : List[RegistrationTransformation]
    original_transformations : List[RegistrationTransformation]
    camera_positions : List[Vector3]
    pre_analysis_results : List[AnalysisResults]
    results : List[AnalysisResults]
    verbose : bool
    show_plot : bool
    nCamera : int
    change : List[Tuple[Vector3, Vector3]]
    proposed_cellsize_factor : float
    proposed_cellsize : float
    # Optional functionality
    correspondence : Optional[float]
    randomize_floor : bool

    def __init__(self):
        MulticamAlignmentAlgorithm.__init__(self)
        BaseMulticamAlgorithm.__init__(self)
        self.original_pointcloud = None
        self.transformations  = []
        self.original_transformations = []
        self.camera_positions = []
        self.pre_analysis_results = []
        self.results = []
        self.aligner_class = DEFAULT_FINE_ALIGNMENT_ALGORITHM

        self.is_interactive = False
        self.verbose = False
        self.show_plot = False
        self.nCamera = 0

        self.change = []
        self.proposed_cellsize_factor : float = math.sqrt(2) # math.sqrt(0.5) # xxxjack or 1, or math.sqrt(2)
        self.proposed_cellsize_method : str = "max"
        self.proposed_cellsize : float = 0

        self.correspondence = None
        self.randomize_floor = False

        np.set_printoptions(precision=4, formatter={"float_kind": lambda x: "%.4f" % x}) # xxxjack completely the wrong place to set this
    
    @override
    def set_max_correspondence(self, max_correspondence: float) -> None:
        """Set the maximum correspondence for this algorithm"""
        self.correspondence = max_correspondence

    @override
    def set_tiled_pointcloud(self, pc : cwipc_pointcloud_wrapper) -> None:
        """Add each individual per-camera tile of this pointcloud, to be used during the algorithm run"""
        if self.randomize_floor:
            new_pc = cwipc_randomize_floor(pc)
            super().set_tiled_pointcloud(new_pc)
        else:
            super().set_tiled_pointcloud(pc)

    def _prepare_analyze(self) -> AnalysisAlgorithm:
        analyzer = None
        assert self.analyzer_class
        if self.verbose:
            print(f"{self.__class__.__name__}: Use analyzer class {self.analyzer_class.__name__}")
        analyzer = self.analyzer_class()
        analyzer.verbose = self.verbose
        return analyzer

    def _prepare_aligner(self) -> AlignmentAlgorithm:
        if not self.aligner_class:
            self.aligner_class = DEFAULT_FINE_ALIGNMENT_ALGORITHM
        if self.verbose:
            print(f"{self.__class__.__name__}: Use aligner class {self.aligner_class.__name__}")
        aligner = self.aligner_class()
        aligner.verbose = self.verbose
        return aligner

    @override
    def set_original_transform(self, cam_index : int, matrix : RegistrationTransformation) -> None:
        assert self.original_pointcloud
        nCamera = get_tiles_used(self.original_pointcloud)
        if len(self.transformations) == 0:
            for i in range(self.camera_count()):
                self.transformations.append(transformation_identity())
        self.transformations[cam_index] = matrix

    def _init_transformations(self) -> None:
        # Initialize matrices, if not done already (by our caller calling set_original_transform)
        if len(self.transformations) == 0:
            for i in range(self.camera_count()):
                self.transformations.append(transformation_identity())
        self.original_transformations = copy.deepcopy(self.transformations)
        assert len(self.camera_positions) == 0
        for i in range(self.camera_count()):
            translation = transformation_get_translation(self.transformations[i])
            campos = translation
            self.camera_positions.append(campos)

    def _pre_analyse(self, toSelf=False, toReference : Optional[cwipc_pointcloud_wrapper] = None, onlyFloor : bool = False, ignoreFloor : bool = False, sortBy : str='corr', target_dirfilter : Optional[float] = None) -> None:
        """
        Pre-analyze the pointclouds and returns a list of camera indices in order of best to worst correspondence.
        If toSelf is true the internal nearest-point distances are computed (a measure of the quality of the capture of this camera).
        If toReference is passed in this is used as the ground truth, otherwise every camera is compared to all other cameras combined.
        If ignoreFloor is true any points with Y<0.1 are ignored.
        sortBy can be one of
        - "corr" for lowest correspondence first
        - "corrcount" for highest count for below-correspondence points first
        - "sourcecount" for highest source cloud point count first
        - "none" don't sort
        if target_dirfilter is specified it first filters the target point cloud to only include points with a normal
        in the appropriate direction of the source camera.
        """
        assert self.original_pointcloud
        assert self.camera_count() > 1
        self.pre_analysis_results = []
        for camnum in range(self.camera_count()):
            tilemask = self.tilemask_for_camera_index(camnum)
            othertilemask = 0xff ^ tilemask
            if toSelf or toReference != None:
                analyzer = RegistrationAnalyzer()
                analyzer.verbose = self.verbose
            else:
                analyzer = self._prepare_analyze()
            analyzer.set_source_pointcloud(self.original_pointcloud, tilemask)
            if toReference != None:
                analyzer.set_reference_pointcloud(toReference)
                if onlyFloor:
                    analyzer.apply_source_filter(lambda pc : cwipc_floor_filter(pc, keep=True))
                    analyzer.set_correspondence_measure('q=95')
                    label = "flooronly(q=95)"
                else:
                    analyzer.set_correspondence_measure('median')
                    label = "toreference(median)"
            elif toSelf:
                analyzer.set_reference_pointcloud(self.original_pointcloud, tilemask)
                analyzer.set_ignore_nearest(1) # xxxjack may want to experiment with larger values.
                analyzer.set_correspondence_measure('median')
                label = "precision(median)"
            else:
                analyzer.set_reference_pointcloud(self.original_pointcloud, othertilemask)
                analyzer.set_correspondence_measure('2mode')
                label = "correspondence(2mode)"
            if ignoreFloor:
                analyzer.set_ignore_floor(True)
            if target_dirfilter != None:
                threshold = target_dirfilter
                direction = self.camera_positions[camnum]
                filter = lambda pc : cwipc_direction_filter(pc, direction, threshold)
                analyzer.apply_reference_filter(filter)
                label += f" (dirfilter={target_dirfilter})"
            analyzer.run()
            results = analyzer.get_results()
            self.pre_analysis_results.append(results)

        if sortBy == 'corr':
            self.pre_analysis_results.sort(key=lambda r : r.minCorrespondence)
        elif sortBy == 'corrcount':
            self.pre_analysis_results.sort(key=lambda r : r.minCorrespondenceCount, reverse=True)
        elif sortBy == 'sourcecount':
            self.pre_analysis_results.sort(key=lambda r : r.sourcePointCount, reverse=True)
        elif sortBy == 'none':
            pass
        else:
            assert False, f"Unknown sortBy={sortBy}"

        if self.verbose or self.is_interactive:
            self._print_correspondences(f"{self.__class__.__name__}: Before:  Per-camera capture {label}", self.pre_analysis_results)

        if self.show_plot:
            self._plot(f"{self.__class__.__name__}: Capture {label}", self.pre_analysis_results)

    def _todo_from_pre_analysis_results(self) -> OrderedCameraList:
        rv : OrderedCameraList = []
        for i in range(len(self.pre_analysis_results)):
            r = self.pre_analysis_results[i]
            assert type(r.tilemask) == int
            rv.append((i, r.tilemask, r.minCorrespondence, r.minCorrespondenceCount/r.sourcePointCount))
        return rv
    
    @abstractmethod
    def run(self) -> bool:
        """Run the algorithm"""
        assert False
    
    def _post_analyse(self, toReference : Optional[cwipc_pointcloud_wrapper] = None, onlyFloor=False) -> bool:
        assert self.original_pointcloud
        assert self.original_pointcloud.count() > 0
        assert self.camera_count() > 0
        self.results = []
        for camnum in range(self.camera_count()):
            tilemask = self.tilemask_for_camera_index(camnum)
            othertilemask = 0xff ^ tilemask
            analyzer = self._prepare_analyze()
            analyzer.set_source_pointcloud(self.original_pointcloud, tilemask)
            if toReference:
                analyzer.set_reference_pointcloud(toReference)
                if onlyFloor:
                    analyzer.apply_source_filter(lambda pc : cwipc_floor_filter(pc, keep=True))
                    analyzer.set_correspondence_measure('q=95')
                    label = "flooronly(q=95)"
                else:
                    analyzer.set_correspondence_measure('median')
                    label = "toreference(median)"
            else:
                analyzer.set_reference_pointcloud(self.original_pointcloud, othertilemask)
                analyzer.set_correspondence_measure('mode')
                label = "correspondence(mode)"
            analyzer.run()
            results = analyzer.get_results()
            self.results.append(results)
        
        if self.verbose:
            self._print_correspondences(f"{self.__class__.__name__}: After:  Per-camera {label}", self.results)

        if self.show_plot:
            self._plot(f"{self.__class__.__name__}: After {label}", self.results)
        
        correspondences = [r.minCorrespondence for r in self.results]
        if self.proposed_cellsize_method == "max":
            correspondence = max(correspondences)
        elif self.proposed_cellsize_method == "min":
            correspondence = min(correspondences)
        elif self.proposed_cellsize_method == "avg":
            correspondence = sum(correspondences) / len(correspondences)

        self.proposed_cellsize = correspondence * self.proposed_cellsize_factor

        self._compute_change()
        self._compute_new_tiles()
        return True

    def _compute_change(self):
        print(f"{self.__class__.__name__}: Change in matrices after alignment:")
        for cam_index in range(len(self.transformations)):
            orig_transform  = self.original_transformations[cam_index] # type: ignore
            new_transform  = self.transformations[cam_index] # type: ignore
            translation, rotation = transformation_compare(orig_transform, new_transform)
            tile = self.tilemask_for_camera_index(cam_index)
            translation_dist = numpy.linalg.norm(translation)
            rotation_dist = numpy.linalg.norm(rotation)
            print(f"\ttile={tile}, distance={translation_dist:.4f}, angle={rotation_dist:.1f}, translation={translation}, rotation={rotation}")

            self.change.append((translation, rotation))

    def _compute_new_tiles(self) -> bool:
        assert self.original_pointcloud
        if self.proposed_cellsize == 0:
            print(f"{self.__class__.__name__}: Warning: proposed_cellsize==0. Cannot compute new tiles.")
            return False
        if True or self.verbose:
            print(f"{self.__class__.__name__}: Computing tile occupancy with {self.proposed_cellsize}")
        tilenum_and_pointcount = cwipc_compute_tile_occupancy(self.original_pointcloud, cellsize=self.proposed_cellsize, filterfloor=True)
        if True or self.verbose:
            print(f"{self.__class__.__name__}: Pointcounts per tile, after voxelizing:")
            for tile, pointcount in tilenum_and_pointcount:
                print(f"\ttile {tile}: {pointcount} ({tile.bit_count()} contributors)")
        return True

    @override
    def get_result_transformations(self) -> List[RegistrationTransformation]:
        return self.transformations
    
    @override
    def get_result_pointcloud_full(self) -> cwipc_pointcloud_wrapper:
        assert self.original_pointcloud
        return self.original_pointcloud
  
    def _plot(self, title : str, results : List[AnalysisResults]) -> None:
        # xxxjack removing this so we may get camera order:
        # results.sort(key=lambda r: (r.minCorrespondence))
        plotter = Plotter(title=title)
        plotter.set_results(results)
        plotter.plot(show=True)
       
    def _print_correspondences(self, label: str, results : List[AnalysisResults]) -> None:
        # xxxjack better not to sort. results.sort(key=lambda r: (r.minCorrespondence))
        print(f"{label}:")
        for i in range(len(results)):
            r = results[i]
            print(f"\tcamnum={r.tilemask}, reference={r.referenceTilemask}, {r.tostr()}")
    
class MultiCameraOneToAllOthers(BaseMulticamAlignmentAlgorithm):
    """\
    Align multiple cameras. Every step, one camera is aligned to all others.
    Every step, we pick the camera with the best chances to make the biggest change.
    """
    # precision_threshold : float


    def __init__(self):
        super().__init__()
        
    @override
    def run(self) -> bool:
        """Run the algorithm"""
        assert self.original_pointcloud
        assert self.camera_count() > 0
        self._init_transformations()
        self._pre_analyse(toSelf=False)
        todo = self._todo_from_pre_analysis_results()

        for camnum, tilemask, corr, fraction in todo:
            aligner = self._prepare_aligner()
            othertilemask = 0xff ^ tilemask
            aligner.set_source_pointcloud(self.original_pointcloud, tilemask)
            aligner.set_reference_pointcloud(self.original_pointcloud, othertilemask)
            if self.correspondence is not None:
                corr = self.correspondence
            aligner.set_correspondence(corr)
            aligner.run()
            
            # Remember resultant pointcloud
            new_pc = aligner.get_result_pointcloud_full()
            self.original_pointcloud = new_pc

            # Apply new transformation (to the left of the old one)
            old_transform = self.transformations[camnum]
            this_transform = aligner.get_result_transformation()
            new_transform = np.matmul(this_transform, old_transform)
            self.transformations[camnum] = new_transform

        ok = self._post_analyse()
        return ok

class MultiCameraToFloor(BaseMulticamAlignmentAlgorithm):
    """\
    Align multiple cameras to the floor at Y=0. Requires enough floor to be visible for each camera.
    A synthetic floor is computed by projecting all points to Y=0.
    Subsequently each camera is aligned to that floor with a max correspondence of `mode`.
    """
    # precision_threshold : float


    def __init__(self):
        super().__init__()
        # self.precision_threshold = 0.001 # Don't attempt to re-align better than 1mm
        self.floor_pointcloud : Optional[cwipc_pointcloud_wrapper] = None

    @override
    def run(self) -> bool:
        """Run the algorithm"""
        assert self.original_pointcloud
        assert self.camera_count() > 0
        self._init_transformations()
        self._prepare_floor()
        assert self.floor_pointcloud
        self._pre_analyse(toSelf=False, toReference=self.floor_pointcloud, onlyFloor=True, sortBy='none')
        todo = self._todo_from_pre_analysis_results()
        aligned : List[cwipc_pointcloud_wrapper] = []
        # xxxjack remember resultant point clouds, to combine later.
        for camnum, tilemask, corr, fraction in todo:
            aligner = self._prepare_aligner()
            aligner.set_source_pointcloud(self.original_pointcloud, tilemask)
            aligner.set_reference_pointcloud(self.floor_pointcloud)
            if self.correspondence is not None:
                corr = self.correspondence
            aligner.set_correspondence(corr)
            aligner.run()
            
            aligned.append(aligner.get_result_pointcloud())

            # Apply new transformation (to the left of the old one)
            old_transform = self.transformations[camnum]
            this_transform = aligner.get_result_transformation()
            new_transform = np.matmul(this_transform, old_transform)
            self.transformations[camnum] = new_transform
        result = functools.reduce(lambda pc1, pc2 : cwipc_join(pc1, pc2), aligned)
        self.original_pointcloud = result

        ok = self._post_analyse(toReference=self.floor_pointcloud, onlyFloor=True)
        return ok

    def _prepare_floor(self) -> None:
        assert self.original_pointcloud
        ndarray = self.original_pointcloud.get_numpy_matrix()
        ndarray[:,1] = 0
        self.floor_pointcloud = cwipc_from_numpy_matrix(ndarray, 0)

    @override
    def _compute_new_tiles(self) -> bool:
        return False
    
class MultiCameraToGroundTruth(BaseMulticamAlignmentAlgorithm):
    """\
    Align multiple cameras to a ground truth which needs to be specified with set_groundtruth().
    Each camera is aligned to that ground truth with a max correspondence of `mode`.
    """
    # precision_threshold : float


    def __init__(self):
        super().__init__()
        # self.precision_threshold = 0.001 # Don't attempt to re-align better than 1mm
        self.groundtruth_pointcloud : Optional[cwipc_pointcloud_wrapper] = None

    def set_groundtruth(self, pc : cwipc_pointcloud_wrapper):
        self.groundtruth_pointcloud = pc

    @override
    def run(self) -> bool:
        """Run the algorithm"""
        assert self.original_pointcloud
        assert self.groundtruth_pointcloud
        assert self.camera_count() > 0
        self._init_transformations()
        self._pre_analyse(toSelf=False, toReference=self.groundtruth_pointcloud, ignoreFloor=True, sortBy='none')
        todo = self._todo_from_pre_analysis_results()
        aligned : List[cwipc_pointcloud_wrapper] = []
        # xxxjack remember resultant point clouds, to combine later.
        for camnum, tilemask, corr, fraction in todo:
            aligner = self._prepare_aligner()
            aligner.set_source_pointcloud(self.original_pointcloud, tilemask)
            aligner.set_reference_pointcloud(self.groundtruth_pointcloud)
            if self.correspondence is not None:
                corr = self.correspondence
            aligner.set_correspondence(corr)
            aligner.run()
            
            aligned.append(aligner.get_result_pointcloud())

            # Apply new transformation (to the left of the old one)
            old_transform = self.transformations[camnum]
            this_transform = aligner.get_result_transformation()
            new_transform = np.matmul(this_transform, old_transform)
            self.transformations[camnum] = new_transform
        result = functools.reduce(lambda pc1, pc2 : cwipc_join(pc1, pc2), aligned)
        self.original_pointcloud = result

        ok = self._post_analyse(toReference=self.groundtruth_pointcloud)
        return ok

    @override
    def _compute_new_tiles(self) -> bool:
        return False

class MultiCameraIterative(BaseMulticamAlignmentAlgorithm):
    """\
    Align multiple cameras. The first step we pick the camera with the best overal to all others.
    We move this to the destination set.

    Next we pick a camera with the best overlap with the destination set and align it to the destination set.
    We repeat this until all cameras are aligned.
    """
    current_step_target_pointcloud : Optional[cwipc_pointcloud_wrapper]
    current_step_in_pointcloud : Optional[cwipc_pointcloud_wrapper]
    current_step_out_pointcloud : Optional[cwipc_pointcloud_wrapper]
    current_step_results : List[AnalysisResults]

    def __init__(self):
        super().__init__()
        self.current_step_target_pointcloud = None
        self.current_step_in_pointcloud = None
        self.current_step_out_pointcloud = None
        self.remaining_results : List[AnalysisResults] = []
        # Optional functionality
        self.orientation_filter : Optional[float] = -0.3
        self.select_target_tile : bool = False
        self.randomize_floor : bool = True
        self.candidate_measure : str = "2mode"
    
    def _pre_step_analyse(self, stepnum : int) -> None:
        """
        Analyze the remaining camera pointclouds for how well they match the current result. Returns a list of camera indices in order of best to worst correspondence.
        """
        assert self.original_pointcloud
        assert self.current_step_target_pointcloud
        assert self.camera_count() > 1
        old_remaining_results = self.remaining_results
        assert old_remaining_results
        remaining_results : List[AnalysisResults] = []
        for rr in old_remaining_results:
            tilemask = rr.tilemask
            assert type(tilemask) == int
            analyzer = self._prepare_analyze()
            analyzer.set_ignore_floor(True)
            analyzer.set_source_pointcloud(self.original_pointcloud, tilemask)
            analyzer.set_reference_pointcloud(self.current_step_target_pointcloud)
            analyzer.set_correspondence_measure(self.candidate_measure, "tmean", "mean")
            if self.orientation_filter != None:
                threshold = self.orientation_filter
                camnum = self.camera_index_for_tilemask(tilemask)
                direction = self.camera_positions[camnum]
                filter = lambda pc : cwipc_direction_filter(pc, direction, threshold)
                analyzer.apply_reference_filter(filter)
            analyzer.run()
            results = analyzer.get_results()
            remaining_results.append(results)

        remaining_results.sort(key=lambda rr: rr.minCorrespondence)
        
        if self.verbose or self.is_interactive:
            label = f"{self.__class__.__name__}: Step {stepnum}:  Per-tile correspondence to target"
            if self.orientation_filter != None:
                label += f" (dirfilter={self.orientation_filter})"
            self._print_correspondences(label, remaining_results)
        self.remaining_results = remaining_results

    def _get_pre_step_result_for_tilemask(self, tilemask : int) -> AnalysisResults:
        for rr in self.remaining_results:
            if rr.tilemask == tilemask:
                return rr
        assert False, "No remaining_results for {tilemask}"

    def _post_step_analyse(self, stepnum : int, camnum : int) -> List[AnalysisResults]:
        """
        Analyze the alignment before and after this step.
        Will be used to judge whether the step was successful.
        """
        rv : List[AnalysisResults] = []
        assert self.original_pointcloud
        assert self.current_step_target_pointcloud
        assert self.current_step_in_pointcloud
        assert self.current_step_out_pointcloud
        rv : List[AnalysisResults] = []

        analyzer = self._prepare_analyze()
        analyzer.set_source_pointcloud(self.current_step_in_pointcloud)
        analyzer.set_reference_pointcloud(self.current_step_target_pointcloud)
        analyzer.set_ignore_floor(True)
        analyzer.set_correspondence_measure("2mode", "tmean", "median")
        # xxxjack should we apply target_filter?
        analyzer.run()
        results = analyzer.get_results()
        results.tilemask = f"{results.tilemask} before"
        rv.append(results)

        analyzer = self._prepare_analyze()
        analyzer.set_ignore_floor(True)
        analyzer.set_source_pointcloud(self.current_step_out_pointcloud)
        analyzer.set_reference_pointcloud(self.current_step_target_pointcloud)
        analyzer.set_ignore_floor(True)
        analyzer.set_correspondence_measure("2mode", "tmean", "median")
        # xxxjack should we apply target filter?
        analyzer.run()
        results = analyzer.get_results()
        results.tilemask = f"{results.tilemask} after"
        rv.append(results)

        if self.verbose or self.is_interactive:
            label = f"{self.__class__.__name__}: Step {stepnum}: camnum {camnum}: Pre/post correspondences"
            if self.orientation_filter != None:
                label += f" (dirfilter={self.orientation_filter})"
            self._print_correspondences(label, rv)

        return rv

    def _accept_step(self, step: int, aligner : AlignmentAlgorithm) -> Tuple[bool, bool]:
        """Allows subclasses to accept the result of this step (or not) and to give up (or continue)"""
        old_rr = self.current_step_results[0]
        new_rr = self.current_step_results[1]
        corr_improvement = old_rr.minCorrespondence / new_rr.minCorrespondence
        corr_count_improvement = new_rr.minCorrespondenceCount / old_rr.minCorrespondenceCount
        if corr_improvement >= 0.99 and corr_count_improvement >= 0.99:
            accept = True
            print(f"{self.__class__.__name__}: Step {step}: very good, accept, tile={old_rr.tilemask}, improvement={corr_improvement:.2f}, count_improvement={corr_count_improvement:.2f}")
        elif corr_improvement >= 0.8 and corr_count_improvement >= 0.8 and corr_improvement * corr_count_improvement >= 1:
            accept = True
            print(f"{self.__class__.__name__}: Step {step}: good overall, accept, tile={old_rr.tilemask}, improvement={corr_improvement:.2f}, count_improvement={corr_count_improvement:.2f}")
        elif corr_improvement >= 2 and corr_improvement * corr_count_improvement >= 2:
            accept = True
            # xxxjack this needs work. We need some other way to judge what has happened.
            print(f"{self.__class__.__name__}: Step {step}: great (but at cost of count), accept, tile={old_rr.tilemask}, improvement={corr_improvement:.2f}, count_improvement={corr_count_improvement:.2f}")
        elif corr_improvement >= 1.5 and corr_improvement * corr_count_improvement >= 1.5:
            accept = True
            # xxxjack this needs work. We need some other way to judge what has happened.
            print(f"{self.__class__.__name__}: Step {step}: borderline, accept, tile={old_rr.tilemask}, improvement={corr_improvement:.2f}, count_improvement={corr_count_improvement:.2f}")
        else:
            accept = False
            print(f"{self.__class__.__name__}: Step {step}: bad, reject, tile={old_rr.tilemask}, improvement={corr_improvement:.2f}, count_improvement={corr_count_improvement:.2f}")
        return accept, False
    
    def _done_step(self, step : int, tilemask : int) -> bool:
        """This tile has been aligned (and accepted). Remove from todo list."""
        for i in range(len(self.remaining_results)):
            if self.remaining_results[i].tilemask == tilemask:
                del self.remaining_results[i]
                return True
        assert False, f"Tilemask {tilemask} not in self.remaining_results"

    def _select_first_step(self) -> int:
        """Select first camera, to align others to. Returns tilemask. Can be overridden by subclasses"""
        rr = self.pre_analysis_results[0]
        assert type(rr.tilemask) == int
        print(f"{self.__class__.__name__}: Step 0: tile={rr.tilemask}")
        return rr.tilemask
    
    def _select_next_step(self, step : int) -> Tuple[int, float, Optional[int]]:
        """Select next tile to align. Can be overridden by subclasses."""
        rr = self.remaining_results[0]
        assert type(rr.tilemask) == int
        rv = (rr.tilemask, rr.minCorrespondence, None)
        print(f"{self.__class__.__name__}: Step {step}: tile={rr.tilemask}, corr={rr.minCorrespondence:.4f}")
        return rv
    
    def _still_to_do(self) -> List[int]:
        """Return list of tilemasks that still need to be aligned"""
        rv : List[int] = list([rr.tilemask for rr in self.remaining_results]) # type: ignore
        return rv
    
    def _downsample_size(self) -> float:
        return 0
    
    def _optional_apply_floor_filter(self) -> None:
        pass

    @override
    def run(self) -> bool:
        """Run the algorithm"""
        assert self.original_pointcloud
        assert self.camera_count() > 0
        self._init_transformations()
        self._pre_analyse(toSelf=True, ignoreFloor=True, sortBy='corr')
        self._pre_analyse(toSelf=False, ignoreFloor=True, sortBy='corr')

        cellsize = self._downsample_size()
        if cellsize > 0:
            self.original_pointcloud = cwipc_downsample_pertile(self.original_pointcloud, cellsize)
            self._pre_analyse(toSelf=True, ignoreFloor=True, sortBy='corr')
            self._pre_analyse(toSelf=False, ignoreFloor=True, sortBy='corr')
        # The first point cloud we keep as-is, and use it as the destination set.
        first_tilemask = self._select_first_step()
        self.remaining_results = copy.copy(self.pre_analysis_results)
        self._done_step(0, first_tilemask)
        if self.verbose:
            print(f"{self.__class__.__name__}: First tilemask (not aligned) is {first_tilemask}")
        self.current_step_target_pointcloud = self.get_pc_for_tilemask(first_tilemask)
        step = 0
        give_up = False
        failures_this_step = 0
        need_new_analysis = True
        while self.remaining_results and not give_up:
            assert self.current_step_target_pointcloud
            assert self.current_step_target_pointcloud.count() > 0
            step += 1
            if need_new_analysis:
                self._pre_step_analyse(step)
            tilemask, corr, targettile = self._select_next_step(step)
            if self.correspondence is not None:
                corr = self.correspondence
            if self.verbose:
                ttile = "" if targettile is None else f", targettile={targettile}"
                print(f"{self.__class__.__name__}: Step {step}: Next tilemask to align is {tilemask}. corr={corr}{ttile}")
            self.current_step_in_pointcloud = self.get_pc_for_tilemask(tilemask)
            self._optional_apply_floor_filter()
            aligner = self._prepare_aligner()
            aligner.set_source_pointcloud(self.current_step_in_pointcloud)
            aligner.set_reference_pointcloud(self.current_step_target_pointcloud, targettile)
            aligner.set_correspondence(corr)
            if self.orientation_filter != None:
                threshold = self.orientation_filter
                camnum = self.camera_index_for_tilemask(tilemask)
                direction = self.camera_positions[camnum]
                filter = lambda pc : cwipc_direction_filter(pc, direction, threshold)
                aligner.apply_reference_filter(filter)
            aligner.run()

            self.current_step_out_pointcloud = aligner.get_result_pointcloud()
            self.current_step_results = self._post_step_analyse(step, tilemask)
            if self.verbose or self.is_interactive:
                # show change in understandable terms
                new_transform = aligner.get_result_transformation()
                translation, rotation = transformation_compare(None, new_transform)
                translation_dist = numpy.linalg.norm(translation)
                rotation_dist = numpy.linalg.norm(rotation)
                print(f"{self.__class__.__name__}: Step {step}: change: distance={translation_dist:.4f}, angle={rotation_dist:.1f}, translation={translation}, rotation={rotation}")

            accept_step, give_up = self._accept_step(step, aligner)
            if accept_step:
                failures_this_step = 0
                need_new_analysis = True
                if self.verbose or self.is_interactive:
                    print(f"{self.__class__.__name__}: Step {step}: accepted alignment for camnum={tilemask}")
                self._done_step(step, tilemask)
                new_resultant_pc = aligner.get_result_pointcloud_full()
                self.current_step_target_pointcloud = None
                self.current_step_in_pointcloud = None
                self.current_step_out_pointcloud = None
                self.current_step_target_pointcloud = new_resultant_pc
                camnum = self.camera_index_for_tilemask(tilemask)
                # Apply new transformation (to the left of the old one)
                old_transform = self.transformations[camnum]
                this_transform = aligner.get_result_transformation()
                new_transform = np.matmul(this_transform, old_transform)
                self.transformations[camnum] = new_transform
            elif not give_up:
                failures_this_step += 1
                need_new_analysis = False
                if True or self.verbose or self.is_interactive:
                    print(f"{self.__class__.__name__}: Step {step}: failed for camnum={tilemask}")
                self.current_step_in_pointcloud = None
                self.current_step_out_pointcloud = None
                # If we have tried everything we give up.
                if failures_this_step > len(self.remaining_results) + 1:
                    print(f"{self.__class__.__name__}: failed {failures_this_step} times.")
                    if not self.is_interactive:
                        give_up = True
                # Re-arrange remaining_results so we try something else.
                first_rr = self.remaining_results.pop(0)
                self.remaining_results.append(first_rr)

        # If we gave up there are still tiles in self.remaining_results that we have to merge into the
        # resultant full point cloud
        to_merge = self._still_to_do()
        for tilemask in to_merge:
            tile_pc = self.get_pc_for_tilemask(tilemask)
            new_pc = cwipc_join(self.current_step_target_pointcloud, tile_pc)
            self.current_step_target_pointcloud = new_pc

        assert self.current_step_target_pointcloud
        assert self.current_step_target_pointcloud.count() > 0
        self.original_pointcloud = self.current_step_target_pointcloud
        self.current_step_target_pointcloud = None

        ok = self._post_analyse()
        return ok

class MultiCameraIterativeInteractive(MultiCameraIterative):
    """\
    Similar to MultiCameraIterative, but before every step the user can change the choices made.
    After each step the user can decide to accept it, or reject it and try something else.
    Additionally the user can decide to give up, which means keeping the registrations made so far but not
    attempting any more."""

    def __init__(self):
        super().__init__()
        self.is_interactive = True
   
    @override
    def _downsample_size(self) -> float:
        return float(self._ask("Downsample size (0 for no downsampling)", 0.0))

    @override
    def _optional_apply_floor_filter(self) -> None:
        assert self.current_step_target_pointcloud
        assert self.current_step_in_pointcloud
        target_radius, target_nonfloor_radius, target_floor_radius = cwipc_compute_radius(self.current_step_target_pointcloud)
        source_radius, source_nonfloor_radius, source_floor_radius = cwipc_compute_radius(self.current_step_in_pointcloud)
        print(f"{self.__class__.__name__}: Step: target radius: all={target_radius}, nonfloor={target_nonfloor_radius}, floor={target_floor_radius}")
        print(f"{self.__class__.__name__}: Step: source radius: all={source_radius}, nonfloor={source_nonfloor_radius}, floor={source_floor_radius}")
        radius = float(self._ask("Radius for floorfilter (0 for no filtering)", 0.0))
        if radius > 0:
            filtered_pc = cwipc_limit_floor_to_radius(self.current_step_in_pointcloud, radius)
            self.current_step_in_pointcloud = filtered_pc

    @override
    def _accept_step(self, step : int, aligner : AlignmentAlgorithm) -> Tuple[bool, bool]:
        accept, giveup = super()._accept_step(step, aligner)
        print(f"{self.__class__.__name__}: Step {step}: automatic decision: {'accept' if accept else 'reject'}, {'give up' if giveup else 'continue'}")
        while True:
            answer = self._ask("Accept this result (yes/no/giveup/show/plot)", "no default")
            if answer == "yes":
                return True, False
            if answer == "no":
                return False, False
            if answer == "giveup":
                return False, True
            if answer == "show":
                self._show_alignment(aligner)
            if answer == "plot":
                self._plot_alignment()

    def _show_alignment(self, aligner : AlignmentAlgorithm):
        assert self.current_step_in_pointcloud
        assert self.current_step_out_pointcloud
        colored_target = cwipc_colormap(aligner.get_filtered_reference_pointcloud(), 0xFFFFFFFF, 0x80808080)
        colored_in = cwipc_colormap(self.current_step_in_pointcloud, 0xFFFFFFFF, 0x80AA0000)
        combined = cwipc_join(colored_target, colored_in)
        colored_out = cwipc_colormap(self.current_step_out_pointcloud, 0xFFFFFFFF, 0x8000AA00)
        combined2 = cwipc_join(combined, colored_out)
        show_pointcloud("Pre and Post of this step", combined2)

    def _plot_alignment(self):
        assert len(self.current_step_results) == 2
        plotter = Plotter(title="Step results")
        plotter.set_results(self.current_step_results)
        plotter.plot(show=True)

    @override
    def _select_first_step(self):
        tilemask = super()._select_first_step()
        assert self.original_pointcloud
        options = list([self.tilemask_for_camera_index(camnum) for camnum in range(self.camera_count())]) + ["show", "plot"]
        while True:
            tilemask_str = self._ask("Tilemask to use as reference", tilemask, options=options)
            if tilemask_str == "show":
                pc_to_show = cwipc_colorized_copy(self.original_pointcloud)
                show_pointcloud("Captured point cloud", pc_to_show)
            elif tilemask_str == "plot":
                plotter = Plotter(title="Pre-analysis results")
                plotter.set_results(self.pre_analysis_results)
                plotter.plot(show=True)
            else:
                tilemask = int(tilemask_str)
                return tilemask
    
    @override
    def _select_next_step(self, step : int) -> Tuple[int, float, Optional[int]]:
        tilemask, corr, ttile = super()._select_next_step(step)
        options = self._still_to_do() + ["plot"]
        while True:
            answer = self._ask("Tilemask to align", tilemask, options=options)
            if answer == "plot":
                plotter = Plotter(title="Candidates")
                plotter.set_results(self.remaining_results)
                plotter.plot(show=True)
            else:
                tilemask = int(answer)
                break
        rr = self._get_pre_step_result_for_tilemask(tilemask)
        if False:
            assert rr.mean
            assert rr.stddev
            corr = rr.mean + rr.stddev
        else:
            corr = rr.minCorrespondence
        corr = float(self._ask("Max correspondence", str(corr)))
        assert self.current_step_target_pointcloud
        ttile = None
        if self.select_target_tile:
            target_tiles = get_tiles_used(self.current_step_target_pointcloud)
            if len(target_tiles) > 1:
                ttile_str = self._ask(f"Target tilemask to align to", "all", options=target_tiles)
                ttile = None if ttile_str == "all" else int(ttile_str)
        return tilemask, corr, ttile

    def _ask(self, prompt : str, default : Any, options : List[Any] = []) -> Any:
        option_str = ""
        if options:
            option_str_list = [str(o) for o in options]
            if not default in options:
                option_str_list.append(str(default))
            option_str = " / ".join(option_str_list)
            option_str = f"( {option_str} ) "
        sys.stdout.write(f"{prompt} {option_str}[{default}] ? ")
        sys.stdout.flush()
        line = sys.stdin.readline()
        line = line.strip()
        if not line: 
            return default
        return line
    
DEFAULT_MULTICAMERA_ALGORITHM = MultiCameraIterative

ALL_MULTICAMERA_ALGORITHMS = [
    MultiCameraOneToAllOthers,
    MultiCameraToFloor,
    MultiCameraIterative,
    MultiCameraIterativeInteractive,
    MultiCameraToGroundTruth
]


HELP_MULTICAMERA_ALGORITHMS = """

## Multicamera algorithms
 
The multicamera algorithm --algorithm_multicamera tries to align multiple cameras to each 
other. It uses an alignment algorithm repeatedly, and an analysis algorithm to determine 
the effect of an alignment.

The various multicamera algorithms differ in the way they select the cameras to align, and
what to try and align it to (either all other cameras, or all cameras that have been 
previously aligned).

Default multicamera algorithm is """ + DEFAULT_MULTICAMERA_ALGORITHM.__name__ + """.

The following multicamera algorithms are available:

""" + "\n".join([f"\t{alg.__name__}\n{algdoc(alg, 2)}" for alg in ALL_MULTICAMERA_ALGORITHMS])