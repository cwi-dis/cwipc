import time
import numpy.random
from typing import Union, List, Tuple, Optional, Dict, Sequence, Any
from .abstract import cwipc_abstract_filter
from ..util import cwipc_pointcloud_wrapper, cwipc_from_numpy_matrix



class NoiseFilter(cwipc_abstract_filter):
    """
    noise - Add noise to the point coordinates.
        Arguments:
            distance: Each point will be moved along a random vector with length up to this distance.
    """
    filtername = "noise"

    def __init__(self, distance : float):
        self.distance = distance
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
        point_matrix = pc.get_numpy_matrix()
        xyz_matrix = point_matrix[:, :3]
        count = xyz_matrix.shape[0]
        noise = self._get_random_vectors(count)
        xyz_matrix += noise
        point_matrix[:, :3] = xyz_matrix
        new_pc = cwipc_from_numpy_matrix(point_matrix, pc.timestamp())
        new_pc._set_cellsize(pc.cellsize())
        pc = new_pc
        t2_d = time.time()
        self.times.append(t2_d-t1_d)
        return pc
    
    def _get_random_vectors(self, count: int):
        """Generate random vectors for noise."""
        rnd_vec = numpy.random.uniform(-1, 1, (count, 3))
        unif = numpy.random.uniform(0, 1, count)
        scale_f = numpy.expand_dims(numpy.linalg.norm(rnd_vec, axis=1)/unif, axis=1)
        rnd_vec = rnd_vec / scale_f
        return rnd_vec * self.distance
    
    def statistics(self) -> None:
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

CustomFilter=NoiseFilter
