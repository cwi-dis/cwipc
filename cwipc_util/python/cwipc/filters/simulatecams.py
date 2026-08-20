import time
import numpy.random
from typing import Union, List, Tuple, Optional, Dict, Sequence, Any
from .abstract import cwipc_abstract_filter
from ..util import cwipc_pointcloud_wrapper, cwipc_from_numpy_matrix



class SimulatecamsFilter(cwipc_abstract_filter):
    """
    simulatecams - Turn point cloud into multiple tiles by simulating cameras.
        Arguments:
            ncam: The number of cameras, spaced equidistantly on a circle around x=z=0.
            hard: If False or not specified, each point is assigned to the camera with the highest dot product
                  or the second highest dot product, with a probability proportional to the dot products.
                  If True, each point is assigned to the camera with the highest dot product.
            skew: If hard=False a skew > 1 will skew the distribution to the closest camera.
    """
    filtername = "simulatecams"

    def __init__(self, ncamera : int, hard : Optional[bool] = False, skew : Optional[float] = 1.0):
        self.ncamera = ncamera
        self.camera_vectors = numpy.zeros((ncamera, 3), dtype=float)
        for i in range(ncamera):
            angle = 2 * numpy.pi * i / ncamera
            self.camera_vectors[i, 0] = numpy.cos(angle)
            self.camera_vectors[i, 1] = 0.0
            self.camera_vectors[i, 2] = numpy.sin(angle)
        self.hard = hard
        self.skew = skew
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
        points = point_matrix[:, :3]
        centroid = numpy.mean(points, axis=0)
        centroid[1] = 0.0  # Set y to 0 to simulate a horizontal plane

        count = point_matrix.shape[0]
        for i in range(count):
            point_vector = numpy.array(point_matrix[i, :3])
            point_vector[1] = 0.0  # Set y to 0 to simulate a horizontal plane
            point_vector -= centroid  # Center the point around the centroid
            dot_products = numpy.zeros(self.ncamera, dtype=float)
            for camera_index in range(self.ncamera):
                camera_vector = self.camera_vectors[camera_index]
                dot_products[camera_index] = numpy.dot(point_vector, camera_vector)
            camera_indices = numpy.argsort(dot_products)[::-1]  # Find the ordering of cameras by highest dot product
            if self.hard:
                camera_index = camera_indices[0]  # Select the camera with the lowest dot product
            else:
                camera_index_0 = camera_indices[0]  # Camera with highest dot product
                camera_index_1 = camera_indices[1]  # Camera with second highest dot product
                weight_0 = dot_products[camera_index_0] ** self.skew
                weight_1 = dot_products[camera_index_1] ** self.skew
                chance = numpy.random.uniform(-weight_0, weight_1)
                if chance < 0:
                    camera_index = camera_index_0
                else:
                    camera_index = camera_index_1
            point_matrix[i, 6] = 1 << camera_index  # Assign camera index to the tile number

        new_pc = cwipc_from_numpy_matrix(point_matrix, pc.timestamp())
        new_pc._set_cellsize(pc.cellsize())
        pc = new_pc
        t2_d = time.time()
        self.times.append(t2_d-t1_d)
        return pc
    
    
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

CustomFilter=SimulatecamsFilter
