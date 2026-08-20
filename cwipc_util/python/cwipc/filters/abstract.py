from abc import ABC, abstractmethod
from ..util import cwipc_pointcloud_wrapper

class cwipc_abstract_filter(ABC):

    @abstractmethod
    def filter(self, pc: cwipc_pointcloud_wrapper) -> cwipc_pointcloud_wrapper:
        """Feed a point cloud to the filter. Returns the resulting point cloud.
        """
        ...

    def statistics(self) -> None:
        """Print statistics on the usage of the filter. This can be things like total runtime used,
        number of point clouds processed, average point counts per cloud, etc."""
        ...
        
    def set_keep_source(self) -> None:
        """Set the filter to keep the source point cloud instead of freeing it after processing.
        If the filter returns the same point cloud as it received as an argument it will never be freed."""
        pass