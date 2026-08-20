import time
from typing import Union, List
from .abstract import cwipc_abstract_filter
from ..util import cwipc_pointcloud_wrapper
from ..registration.util import transformation_frompython, cwipc_transform

class Transform44Filter(cwipc_abstract_filter):
    """
    transform - Adjust coordinate system of the point clouds.
        Arguments:
            matrix: 4x4 transformation matrix as a list of lists.
    """
    filtername = "transform"

    def __init__(self, matrix: List[List[float]]):
        self.transform = transformation_frompython(matrix)
        self.count = 0
        self.times = []
        self.keep_source = False
        
    def set_keep_source(self) -> None:
        """Set the filter to keep the source point cloud instead of freeing it after processing.
        If the filter returns the same point cloud as it received as an argument it will never be freed."""
        self.keep_source = True        
        
    def filter(self, pc : cwipc_pointcloud_wrapper) -> cwipc_pointcloud_wrapper:
        self.count += 1
        t1_d = time.time()
        newpc = cwipc_transform(pc, self.transform)
        t2_d = time.time()
        self.times.append(t2_d-t1_d)
        return newpc
        
    def statistics(self):
        print(f"transform44: count={self.count}")
        if self.times:
            self.print1stat('duration', self.times)


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

CustomFilter = Transform44Filter
