import time
from typing import Union, List, Tuple
from .abstract import cwipc_abstract_filter
from ..util import cwipc_pointcloud_wrapper
from ..registration.util import cwipc_direction_filter

class DirectionFilter(cwipc_abstract_filter):
    """
    direction - Filter point cloud to points that are approximately oriented in a certain direction.
        Arguments:
            x, y, z: Direction vector
            threshold: float between -1.0 and 1.0. 1.0 is fully aligned with direction, -1.0 is opposite. Default 0.0.
        For each point a normal is computed, based on a surface with adjacent points.
        The dot product between these normals and the direction vector is computed, and any point
        that does not satisfy the threshold is discarded.
        The direction vector is relative to the center of the point cloud.
    """
    filtername = "direction"

    def __init__(self, x : float, y : float, z : float, threshold: float = 0.0):
        self.direction = (x, y, z)
        self.threshold = threshold
        self.count = 0
        self.times = []
        self.original_pointcounts = []
        self.pointcounts = []
        self.keep_source = False
        
    def set_keep_source(self) -> None:
        """Set the filter to keep the source point cloud instead of freeing it after processing.
        If the filter returns the same point cloud as it received as an argument it will never be freed."""
        self.keep_source = True        
        
    def filter(self, pc : cwipc_pointcloud_wrapper) -> cwipc_pointcloud_wrapper:
        self.count += 1
        t1_d = time.time()
        self.original_pointcounts.append(pc.count())
        newpc = cwipc_direction_filter(pc, self.direction, self.threshold)
        t2_d = time.time()
        self.times.append(t2_d-t1_d)
        self.pointcounts.append(newpc.count())
        return newpc
        
    def statistics(self):
        print(f"direction: count={self.count}")
        if self.times:
            self.print1stat('duration', self.times)
        if self.original_pointcounts:
            self.print1stat('original_pointcount', self.original_pointcounts, True)
        if self.pointcounts:
            self.print1stat('pointcount', self.pointcounts, True)


    def print1stat(self, name : str, values : Union[List[int], List[float]], isInt : bool=False) -> None:
        count = len(values)
        if count == 0:
            print(f'{self.filtername}: {name}: count=0')
            return
        minValue = min(values)
        maxValue = max(values)
        avgValue = sum(values) / count
        if isInt:
            fmtstring = '{}: {}: count={}, average={:.3f}, min={:d}, max={:d}'
        else:
            fmtstring = '{}: {}: count={}, average={:.3f}, min={:.3f}, max={:.3f}'
        print(fmtstring.format(self.filtername, name, count, avgValue, minValue, maxValue))

CustomFilter = DirectionFilter
