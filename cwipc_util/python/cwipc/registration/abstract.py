from abc import ABC, abstractmethod
from typing import Optional, Union, Any, List, Tuple, Type, Container, Literal, Callable
import math
import numpy as np
import numpy.typing
from ..abstract import *
from .. import cwipc_pointcloud_wrapper

__all__ = [
    "RegistrationTransformation",
    "Vector3",
    "PointCloudFilter",

    "AnalysisResults",

    "OverlapAnalysisResults",
    "OverlapAnalysisAlgorithm",

    "Algorithm",
    "AnalysisAlgorithm",
    "AlignmentAlgorithm",

    "MulticamAlgorithm",
    "MulticamAlignmentAlgorithm", 

    "AnalysisAlgorithmFactory",
    "AlignmentAlgorithmFactory", 
    "MulticamAlignmentAlgorithmFactory", 
    
]

RegistrationTransformation = np.ndarray[tuple[Literal[4], Literal[4]], np.dtype[np.float64]]
Vector3 = np.ndarray[tuple[Literal[3]], np.dtype[np.float64]]
PointCloudFilter = Callable[[cwipc_pointcloud_wrapper], cwipc_pointcloud_wrapper]

class Algorithm(ABC):
    """Abstract base class for any algorithm that operates on two point clouds.
    Contains the methods for adding the pointclouds, running the algorithm, and returning the result.
    """
    verbose : bool
    debug : bool

    @abstractmethod
    def set_source_pointcloud(self, pc : cwipc_pointcloud_wrapper, tilemask : Optional[int] = None) -> None:
        """Set the source point cloud to be used during the algorithm run"""
        ...
    
    @abstractmethod
    def set_reference_pointcloud(self, pc : cwipc_pointcloud_wrapper, tilemask : Optional[int] = None) -> None:
        """Set the reference point cloud to be used during the algorithm run.
        """
        ...

    @abstractmethod
    def run(self) -> bool:
        """Run the algorithm. Returns false in case of a failure."""
        ...

    @abstractmethod
    def apply_source_filter(self, filter : PointCloudFilter) -> None:
        """Apply a filter to the source point cloud before applying the algorithm"""
        ...

    @abstractmethod
    def apply_reference_filter(self, filter : PointCloudFilter) -> None:
        """Apply a filter to the reference point cloud before applying the algorithm"""
        ...

    @abstractmethod
    def get_source_pointcloud(self) -> cwipc_pointcloud_wrapper:
        """Get original source point cloud"""
        ...

    @abstractmethod
    def get_filtered_source_pointcloud(self) -> cwipc_pointcloud_wrapper:
        """Get filtered source point cloud used during algorithm run"""
        ...
    
    @abstractmethod
    def get_reference_pointcloud(self) -> cwipc_pointcloud_wrapper:
        """Get original reference point cloud"""
        ...
    
    @abstractmethod
    def get_filtered_reference_pointcloud(self) -> cwipc_pointcloud_wrapper:
        """Get filtered point cloud used during algorithm run"""
        ...
class AnalysisResults:
    """Class to hold the results of an analysis algorithm"""
    #: minimum correspondence for each camera. Usually based on next values.
    minCorrespondence : float
    #: number of points that in source cloud that are below minCorrespondence
    minCorrespondenceCount : int

    #: Mean of distances to closest point in other cloud
    mean : Optional[float]
    #: Mean of distances to closest point in other cloud
    stddev : Optional[float]
    #: Trimmed mean of distances to closest point in other cloud. Top and bottom 5% are ignored.
    tmean : Optional[float]
    #: Mode of distances to closest point in other cloud
    mode : Optional[float]
    #: Median of distances to closest point in other cloud
    median : Optional[float]

    #: total number of points in the source point cloud
    sourcePointCount : int
    #: total number of points in the reference point cloud
    referencePointCount : int
    #: tile mask for this analysis data, if applicable
    tilemask : Optional[int | str]
    #: target tilemask, if applicable
    referenceTilemask : Optional[int]
    #: histogram of distances
    histogram : Optional[numpy.typing.NDArray[numpy.float64]]
    #: edges of the histogram
    histogramEdges : Optional[numpy.typing.NDArray[numpy.float64]]

    #: Algorithm used to compute these results
    algorithm : str
    
    #: Algorithm variant used to compute these results
    variant : Optional[str]

    def __init__(self):
        self.minCorrespondence = 0
        self.minCorrespondenceCount = 0
        self.mean = None
        self.stddev = None
        self.tmean = None
        self.mode = None
        self.median = None
        self.sourcePointCount = 0
        self.referencePointCount = 0
        self.tilemask = None
        self.referenceTilemask = None
        self.histogram = None
        self.histogramEdges = None
        self.variant = None

    def tostr(self) -> str:
        """Returns human-readable representation of the statistics"""
        percentage = (self.minCorrespondenceCount / self.sourcePointCount) * 100
        rv : str = f"correspondence: {self.minCorrespondence:.4f}, count: {self.minCorrespondenceCount}, percentage: {percentage:.0f}%"
        if self.mean != None:
            rv += f", mean={self.mean:.4f}"
        if self.stddev != None:
            rv += f", stddev={self.stddev:.4f}"
        if self.tmean != None:
            rv += f", tmean={self.tmean:.4f}"
        if self.mode != None:
            rv += f", mode={self.mode:.4f}"
        if self.median != None:
            rv += f", median={self.median:.4f}"
        return rv

class AnalysisAlgorithm(Algorithm):
    """ABC for a pointcloud analysis algorithm between two point clouds which returns a minimum distance histogram and values"""

    plot_label : Optional[str]
    correspondence_method: Optional[str]

    @abstractmethod
    def set_correspondence_measure(self, method : str, *other_methods : str):
        """Set the algorithm used to comput point cloud correspondence based on point distances.
        Values are mean, median, tmean or mode."""
        ...
        
    @abstractmethod
    def set_max_correspondence_distance(self, correspondence : float) -> None:
        """Set the max correspondence: the maximum distance between two points that are candidates for being "the same" point."""
        ...

    @abstractmethod
    def set_min_correspondence_distance(self, correspondence : float) -> None:
        """Set the min correspondence: the smallest point distance that is meaningful. This value may be used to calculate the
        histogram to be used for the mode algorithm"""
        ...

    @abstractmethod
    def set_ignore_nearest(self, ignore_nearest: int) -> None:
        """Set the number of nearest points to ignore"""
        ...

    @abstractmethod
    def set_ignore_floor(self, ignoreFloor : bool) -> None:
        """Ignore point with a low Y coordinate for the analysis"""
        ...

    @abstractmethod
    def get_results(self) -> AnalysisResults:
        """Returns an object indicating how the source point cloud is aligned to the reference point cloud.
        """
        ...

class OverlapAnalysisResults:
    #: overlapping area (# of inlier correspondences / # points in the source). Higher is better.
    fitness : float
    #: RMSE of all inlier correspondences. Lower is better.
    rmse : float
    #: total number of points in the source point cloud
    sourcePointCount : int
    #: total number of points in the reference point cloud
    referencePointCount : int
    #: tile mask for this analysis data, if applicable
    tilemask : Optional[int]
    #: target tilemask, if applicable
    referenceTilemask : Optional[int]
    
class OverlapAnalysisAlgorithm(Algorithm):
    """ABC for a pointcloud analysis algorithm between two point clouds which returns an overlap indication"""

    @abstractmethod
    def set_correspondence(self, correspondence : float) -> None:
        """Set the correspondence: the maximum distance between two points that are candidates for being "the same" point."""
        ...

    @abstractmethod
    def get_results(self) -> OverlapAnalysisResults:
        """Returns an object indicating how well the two point clouds overlap
        """
        ...

AnalysisAlgorithmFactory = Type[AnalysisAlgorithm]

class AlignmentAlgorithm(Algorithm):
    """ABC for an algorithm that tries to find the best alignment for one tile (or possibly between two tiles, but always returning a new
    matrix for a single tile only)"""

    @abstractmethod
    def set_correspondence(self, correspondence : float) -> None:
        """Set the correspondence: the maximum distance between two points that are candidates for being "the same" point."""
        ...
        
    @abstractmethod
    def get_result_transformation(self) -> RegistrationTransformation:
        """After a successful run(), returns the transformation applied to the tile-under-test"""
        ...
    
    @abstractmethod
    def get_result_pointcloud(self) -> cwipc_pointcloud_wrapper:
        """After a successful run(), returns the point cloud for the tile-under-test after the transformation has been applied"""
        ...
    @abstractmethod
    def get_result_pointcloud_full(self) -> cwipc_pointcloud_wrapper:
         """After a successful run(), returns the point cloud for all tiles combined, after applying transformations"""
         ...

AlignmentAlgorithmFactory = Type[AlignmentAlgorithm]

class MulticamAlgorithm(ABC):
    """Abstract base class for any algorithm that operates on tiled point clouds.
    Contains the methods for adding a pointcloud, converting from tile-index to tile-number and vv, and for running the
    algorithm.
    """
    verbose : bool
    debug : bool

    @abstractmethod
    def set_tiled_pointcloud(self, pc : cwipc_pointcloud_wrapper) -> None:
        """Add each individual per-camera tile of this pointcloud, to be used during the algorithm run"""
        ...
   
    @abstractmethod
    def camera_count(self) -> int:
        """Return number of cameras (tiles) in the point clouds"""
        ...
        
    @abstractmethod
    def tilemask_for_camera_index(self, cam_index : int) -> int:
        """Returns the tilenumber (used in the point cloud) for this index (used in the results)"""
        ...

    @abstractmethod
    def camera_index_for_tilemask(self, tilenum : int) -> int:
        """Returns the  index (used in the results) for this tilenumber (used in the point cloud)"""
        ...
        
#    @abstractmethod
#    def get_pointcloud_for_tilemask(self, tilenum : int) -> cwipc_pointcloud_wrapper:
#        """Returns the point cloud for this tilenumber"""
#        ...

    @abstractmethod
    def run(self) -> bool:
        """Run the algorithm. Returns false in case of a failure."""
        ...
    # There are also methods to return the result, but they don't have a fixed signature.




class MulticamAlignmentAlgorithm(MulticamAlgorithm):
    """ABC for an algorithm that tries to align all tiles."""
    analyzer_class : Optional[AnalysisAlgorithmFactory]
    aligner_class : Optional[AlignmentAlgorithmFactory]

    def __init__(self):
        self.analyzer_class = None
        self.aligner_class = None

    def set_analyzer_class(self, analyzer_class : AnalysisAlgorithmFactory) -> None:
        """Set the class to be used for analyzing the results"""
        self.analyzer_class = analyzer_class

    def set_aligner_class(self, aligner_class : AlignmentAlgorithmFactory) -> None:
        """Set the class to be used for aligning individual tiles"""
        self.aligner_class = aligner_class
    
    def set_max_correspondence(self, max_correspondence: float) -> None:
        """Override maximum correspondence (distance at which potentially matching points are sought)"""
        assert False, f"{self.__class__.__name__} does not implement set_max_correspondence()"

    def set_original_transform(self, cam_index : int, matrix : RegistrationTransformation) -> None:
        """Communicate original matrices to the aligner. Must be overridden if allowed"""
        assert False, f"{self.__class__.__name__} does not implement set_original_transform()"

    @abstractmethod
    def get_result_transformations(self) -> List[RegistrationTransformation]:
        """After a successful run(), returns the list of transformations applied to each tile"""
        ...
    
    @abstractmethod
    def get_result_pointcloud_full(self) -> cwipc_pointcloud_wrapper:
         """After a successful run(), returns the point cloud for all tiles combined, after applying transformations"""
         ...

MulticamAlignmentAlgorithmFactory = Type[MulticamAlignmentAlgorithm]
