from abc import ABC, abstractmethod
from typing import List, Any, Tuple
try:
    from typing import override
except ImportError:
    from typing_extensions import override
from ..abstract import *
from .abstract import *
from .. import cwipc_pointcloud_wrapper, cwipc_from_points, cwipc_from_numpy_array, cwipc_from_numpy_matrix, cwipc_tilefilter, cwipc_downsample, cwipc_join
import open3d
import open3d.visualization
import numpy as np
from numpy.typing import NDArray
import numpy.linalg
from scipy.spatial.transform import RigidTransform
import textwrap

def algdoc(klass : type, indent : int) -> str:
    doc = klass.__doc__
    if doc == None:
        doc = "No documentation available"
    doc = textwrap.dedent(doc)
    doc = textwrap.indent(doc, '\t'*indent)
    return doc

Point_array_xyz = NDArray[Any]
Point_array_rgb = NDArray[Any]

def transformation_identity() -> RegistrationTransformation:
    values  = np.array([
        1, 0, 0, 0,
        0, 1, 0, 0,
        0, 0, 1, 0,
        0, 0, 0, 1
    ], dtype=float)
    rv : RegistrationTransformation = np.reshape(values, (4, 4)) # type: ignore
    return rv

def transformation_invert(orig_transform : RegistrationTransformation) -> RegistrationTransformation:
    """Invert an affine transformation"""
    # Get the 3x3 linear transformation and the translation vector
    orig_matrix = orig_transform[:3, :3]
    orig_translation = orig_transform[:3, 3]
    # Get the inverse linear transformation and the inverse translation
    # Note that we can invert the matrix by transposing because we know it is size-preserving.
    # Also note that the translation vector is in "new coordinates"
    inv_matrix = orig_matrix.T
    inv_translation = -inv_matrix @ orig_translation
    # Put everything together into a 4x4 matrix
    transform = np.empty((4, 4))
    transform[:3, :3] = inv_matrix
    transform[:3, 3] = inv_translation
    transform[3, :] = [0, 0, 0, 1]
    rv : RegistrationTransformation = transform # type: ignore
    return rv

def transformation_frompython(trafo : List[List[float]]) -> RegistrationTransformation:
    rv : RegistrationTransformation = np.array(trafo) # type: ignore
    assert rv.shape == (4, 4)
    return rv

def transformation_topython(matrix : RegistrationTransformation) -> List[List[float]]:
    rv = matrix.tolist()
    assert len(rv) == 4
    assert len(rv[0]) == 4
    return rv

def transformation_get_translation(matrix : RegistrationTransformation) -> Vector3:
    rv : Vector3 = matrix[0:3, 3] # type: ignore
    return rv

def transformation_compare(old : Optional[RegistrationTransformation], new : Optional[RegistrationTransformation]) -> Tuple[Vector3, Vector3]:
    """Returns the translation and rotation (as rotation vector in degrees) that are the difference between old and new"""
    if old is None:
        old = transformation_identity()
    if new is None:
        new = transformation_identity()
    diff = new @ numpy.linalg.inv(old)
    trafo = RigidTransform.from_matrix(diff)
    translation : Vector3 = trafo.translation
    rotation : Vector3 = trafo.rotation.as_rotvec(degrees=True)
    return translation, rotation

def cwipc_center(pc : cwipc_pointcloud_wrapper) -> Tuple[float, float, float]:
    """Compute the center of a point cloud"""
    point_matrix = pc.get_numpy_matrix()
    points = point_matrix[:, :3]
    centroid = np.mean(points, axis=0)
    return tuple(centroid)

def cwipc_colorized_copy(pc : cwipc_pointcloud_wrapper) -> cwipc_pointcloud_wrapper:
    from ..filters import colorize
    cf = colorize.ColorizeFilter(0.8, "camera")
    cf.set_keep_source()
    new_pc = cf.filter(pc)
    return new_pc

def cwipc_tilefilter_masked(pc : cwipc_pointcloud_wrapper, mask : int) -> cwipc_pointcloud_wrapper:
    """Filter a point cloud for specific tiles. Each point tile number is ANDed to the mask, so only points with a tile number that matches the mask are returned.
    The mask is a bitmask."""
    pointarray = pc.get_numpy_array()
    # Extract the relevant fields (X, Y, Z coordinates)
    tilearray = pointarray['tile']
    # Create a mask for the points that match the tile mask
    mask_array = (tilearray & mask) != 0
    # Filter the point cloud using the mask
    filtered_points = pointarray[mask_array]
    if filtered_points.size == 0:
        return cwipc_from_points([], pc.timestamp())
    new_pc = cwipc_from_numpy_array(filtered_points, pc.timestamp())
    new_pc._set_cellsize(pc.cellsize())
    return new_pc

def cwipc_direction_filter(pc : cwipc_pointcloud_wrapper, direction : Vector3|Tuple[float, float, float], threshold : float) -> cwipc_pointcloud_wrapper:
    """Filter a point cloud to keep only points that are somewhat facing a direction."""
    if type(direction) == tuple:
        direction = np.array(list(direction)) # type: ignore
    assert np.shape(direction) == (3,)
    norm = np.linalg.norm(direction)
    if norm != 0:
        direction = direction / norm # type: ignore
    # Get Numpy array of the point cloud
    pc_np = pc.get_numpy_matrix()
    # Use Open3D to guess the normals
    points = pc_np[:, 0:3]
    center = np.mean(points, axis=0)
    o3d_pc = open3d.geometry.PointCloud()
    o3d_pc.points = open3d.utility.Vector3dVector(points)
    strategy = open3d.geometry.KDTreeSearchParamHybrid(radius=0.02, max_nn=30)
    o3d_pc.estimate_normals(strategy)
    # Note: the next line orients the normals in exactly the wrong direction (inwards)
    o3d_pc.orient_normals_towards_camera_location(center)
    normals = np.asarray(o3d_pc.normals)
    normals = -normals # This aligns the normals outwards again.
    # Create a filter based on the normals and the direction vector
    dot_products = normals @ direction
    filter = dot_products >= threshold
    # Filter the points
    pc_np_filtered = pc_np[filter]
    # Create the resultant point cloud
    
    new_pc = cwipc_from_numpy_matrix(pc_np_filtered, pc.timestamp())
    new_pc._set_cellsize(pc.cellsize())
    return new_pc

def cwipc_floor_filter(pc : cwipc_pointcloud_wrapper, level : float = 0.1, keep : bool = False) -> cwipc_pointcloud_wrapper:
    """Remove all points that are probably on the floor (Y ~= 0). If keep=True in stead only keep those points."""
    pc_np = pc.get_numpy_matrix()
    is_floor_point = pc_np[:,1] < level
    if keep:
        new_pc_np = pc_np[is_floor_point]
    else:
        new_pc_np = pc_np[~is_floor_point]
    new_pc = cwipc_from_numpy_matrix(new_pc_np, pc.timestamp())
    return new_pc    

def cwipc_randomize_floor(pc : cwipc_pointcloud_wrapper, level : float = 0.1) -> cwipc_pointcloud_wrapper:
    """Randomly assign all floor points (Y ~= 0) to different tiles."""
    pc_np = pc.get_numpy_matrix()
    is_floor_point = pc_np[:,1] < level
    pc_np_floor = pc_np[is_floor_point]
    pc_np_nonfloor = pc_np[~is_floor_point]
    floor_tiles = pc_np_floor[:,6]
    np.random.shuffle(floor_tiles)
    pc_np_floor[:,6] = floor_tiles
    new_pc_np = np.concatenate((pc_np_floor, pc_np_nonfloor), axis=0)
    new_pc = cwipc_from_numpy_matrix(new_pc_np, pc.timestamp())
    return new_pc

def cwipc_downsample_pertile(pc : cwipc_pointcloud_wrapper, cellsize : float) -> cwipc_pointcloud_wrapper:
    """Per-tile downsample, so points in different tiles are not combined."""
    tiles_used = get_tiles_used(pc)
    result_pc = None
    for tilenum in tiles_used:
        tile_pc = cwipc_tilefilter(pc, tilenum)
        tile_pc_downsampled = cwipc_downsample(tile_pc, cellsize)
        if result_pc == None:
            result_pc = tile_pc_downsampled
        else:
            result_pc = cwipc_join(result_pc, tile_pc_downsampled)
    assert result_pc
    return result_pc

def cwipc_compute_tile_occupancy(pc : cwipc_pointcloud_wrapper, cellsize : float = 0, filterfloor : bool=False) -> List[Tuple[int, int]]:
    """
    Returns list of (tilenum, pointcount) of a point cloud, optionally after downsampling with cellsize and/or removing the floor.
    The list is sorted by point count, and empty tiles are omitted.
    """
    if filterfloor:
        pc = cwipc_floor_filter(pc)
    if cellsize != 0:
        pc = cwipc_downsample(pc, cellsize)
    tiles_used = get_tiles_used(pc)
    rv : List[Tuple[int, int]] = []
    for tilenum in tiles_used:
        pc_tile = cwipc_tilefilter(pc, tilenum)
        pointcount = pc_tile.count()
        rv.append((tilenum, pointcount))
    rv.sort(key=lambda tp : tp[1], reverse=True)
    return rv

def cwipc_compute_radius(pc : cwipc_pointcloud_wrapper, level : float = 0.1) -> Tuple[float, float, float]:
    """Compute the radius in the XZ plane (ignoring outliers). Three numbers are returned,
    the overall radius, the radius ignoring the floor (only points with Y>0.1) and the radius of the floor (only points with Y<0.1)"""
    pc_np = pc.get_numpy_matrix(onlyGeometry=True)
    is_floor_point = pc_np[:,1] < level
    floor_pc_np = pc_np[is_floor_point]
    nonfloor_pc_np = pc_np[~is_floor_point]
    floor_pc_np[:,1] = 0
    nonfloor_pc_np[:,1] = 0
    floor_distances = numpy.linalg.norm(floor_pc_np, axis=1)
    nonfloor_distances = numpy.linalg.norm(nonfloor_pc_np, axis=1)
    floor_max_distance = numpy.percentile(floor_distances, 99)
    nonfloor_max_distance = numpy.percentile(nonfloor_distances, 99)
    max_distance = max(floor_max_distance, nonfloor_max_distance)
    return max_distance, nonfloor_max_distance, floor_max_distance

def cwipc_limit_floor_to_radius(pc : cwipc_pointcloud_wrapper, radius : float, level : float=0.1) -> cwipc_pointcloud_wrapper:
    """Return the point cloud with floor points further away from the Y axis than radius removed"""
    pc_np = pc.get_numpy_matrix()
    is_floor_point = pc_np[:,1] < level
    floor_pc_np = pc_np[is_floor_point]
    nonfloor_pc_np = pc_np[~is_floor_point]
    floor_distances = numpy.linalg.norm(floor_pc_np[:,0:3], axis=1)
    floor_distance_filter = floor_distances < radius
    filtered_floor_pc_np = floor_pc_np[floor_distance_filter]
    new_pc_np = np.concatenate((filtered_floor_pc_np, nonfloor_pc_np), axis=0)
    new_pc = cwipc_from_numpy_matrix(new_pc_np, pc.timestamp())
    return new_pc
    
def show_pointcloud(title : str, pc : Union[cwipc_pointcloud_wrapper, open3d.geometry.PointCloud], from000=False):
    """Show a point cloud in a window. Allow user to interact with it until q is pressed, at which point this method returns.
    
    The point cloud can be either a cwipc or an opend3.geometry.PointCloud.

    The optional from000 argument places the camera at (0, 0, 0).
    """
    if type(pc) == cwipc_pointcloud_wrapper:
        pc_o3d = pc.get_o3d_pointcloud()
        o3d_show_points(title, pc_o3d, from000)
    else:
        o3d_show_points(title, pc, from000)

def o3d_pick_points(title : str, pc : open3d.geometry.PointCloud, from000 : bool=False) -> List[int]:
    """Show a window with an open3d.geometry.PointCloud. Let the user pick points and return the list of point indices picked."""
    vis = open3d.visualization.VisualizerWithEditing() # type: ignore
    vis.create_window(window_name=title, width=1280, height=720)
    #self.winpos += 50
    vis.add_geometry(pc)
    if from000:
        viewControl = vis.get_view_control()
        pinholeCamera = viewControl.convert_to_pinhole_camera_parameters()
        pinholeCamera.extrinsic = transformation_identity()
        viewControl.convert_from_pinhole_camera_parameters(pinholeCamera)
    vis.run() # user picks points
    vis.destroy_window()
    return vis.get_picked_points()

def o3d_show_points(title : str, pc : open3d.geometry.PointCloud, from000=False, keepopen=False) -> Optional[open3d.visualization.Visualizer]:
    """Show a window with an open3d.geometry.PointCloud. """
    vis = open3d.visualization.Visualizer() # type: ignore
    vis.create_window(window_name=title) 
    vis.add_geometry(pc)
    DRAW_OWN_AXES = False
    if DRAW_OWN_AXES:
        # Draw 1 meter axes (x=red, y=green, z=blue)
        axes = open3d.geometry.LineSet()
        axes.points = open3d.utility.Vector3dVector([[0,0,0], [1,0,0], [0,1,0], [0,0,1]])
        axes.lines = open3d.utility.Vector2iVector([[0,1], [0,2], [0,3]])
        axes.colors = open3d.utility.Vector3dVector([[1,0,0], [0,1,0], [0,0,1]])
    else:
        axes = open3d.geometry.TriangleMesh.create_coordinate_frame()
    vis.add_geometry(axes)
    if from000:
        viewControl = vis.get_view_control()
        pinholeCamera = viewControl.convert_to_pinhole_camera_parameters()
        pinholeCamera.extrinsic = transformation_identity()
        viewControl.convert_from_pinhole_camera_parameters(pinholeCamera)
    vis.run()
    if keepopen:
        return vis
    vis.destroy_window()
    return None

def get_tiles_used(pc : cwipc_pointcloud_wrapper) -> List[int]:
    """Return a list of the tile numbers used in the point cloud"""
    pointarray = pc.get_numpy_array()
    # Extract the relevant fields (X, Y, Z coordinates)
    tilearray = pointarray['tile']
    unique = np.unique(tilearray)
    rv : List[int] = unique.tolist()
    rv.sort()
    return rv

def cwipc_transform(pc: cwipc_pointcloud_wrapper, transform : RegistrationTransformation) -> cwipc_pointcloud_wrapper:
    """Apply an affine transpormation to a point cloud and return the resulting point cloud"""

    np_points = pc.get_numpy_matrix()
    n_points = np_points.shape[0]
    np_points_xyz = np_points[...,0:3]
    # Split the affine transform into a rotation and a translation
    rotmat = transform[:3,:3]
    transvec = transform[:3,3].transpose()
    np_points_transformed = (rotmat @ np_points_xyz.transpose()).transpose()
    np_points_transformed = np_points_transformed + transvec
    np_points[...,0:3] = np_points_transformed
    new_pc = cwipc_from_numpy_matrix(np_points, pc.timestamp())
    new_pc._set_cellsize(pc.cellsize())
    return new_pc

class BaseAlgorithm(Algorithm):
    """Base class for most algorithms, both registration and alignment.

    Allows ading point clouds and inspecting them.
    """

    def __init__(self):
        self._source_pointcloud : Optional[cwipc_pointcloud_wrapper] = None
        self._filtered_source_pointcloud : Optional[cwipc_pointcloud_wrapper] = None
        self.source_tilemask : Optional[int] = None
        self._reference_pointcloud : Optional[cwipc_pointcloud_wrapper] = None
        self._filtered_reference_pointcloud : Optional[cwipc_pointcloud_wrapper] = None
        self.reference_tilemask : Optional[int] = None
        self.verbose = False

    @override
    def set_source_pointcloud(self, pc : cwipc_pointcloud_wrapper, tilemask: Optional[int] = None) -> None:
        """Set the source point cloud for this algorithm"""
        pre_count = pc.count()
        if pre_count == 0:
            print(f"{self.__class__.__name__}: set_source_pointcloud: Warning: pre_count={pre_count}")
        if tilemask is not None:
            if tilemask != 0:
                pc = cwipc_tilefilter_masked(pc, tilemask)
            post_count = pc.count()
            if post_count == 0:
                print(f"{self.__class__.__name__}: set_source_pointcloud: Warning: tilemask={tilemask}, post_count={post_count}")
            if self.verbose:
                print(f"{self.__class__.__name__}: Setting source point cloud with {post_count} (of {pre_count}) points using tilemask {tilemask:#x}")
        else:
            if self.verbose:
                print(f"{self.__class__.__name__}: Setting source point cloud with {pre_count} points")

        self._source_pointcloud = pc
        self.source_tilemask = tilemask
    
    @override
    def set_reference_pointcloud(self, pc : cwipc_pointcloud_wrapper, tilemask : Optional[int] = None) -> None:
        """Set the reference point cloud for this algorithm"""
        pre_count = pc.count()
        if pre_count == 0:
            print(f"{self.__class__.__name__}: set_reference_pointcloud: Warning: pre_count={pre_count}")
        if tilemask is not None:
            if tilemask != 0:
                pc = cwipc_tilefilter_masked(pc, tilemask)
            post_count = pc.count()
            if post_count == 0:
                print(f"{self.__class__.__name__}: set_reference_pointcloud: Warning: tilemask={tilemask}, post_count={post_count}")
            if self.verbose:
                print(f"{self.__class__.__name__}: Setting target point cloud with {post_count} (of {pre_count}) points using tilemask {tilemask:#x}")
        else:
            if self.verbose:
                print(f"{self.__class__.__name__}: Setting target point cloud with {pc.count()} points")
        self._reference_pointcloud = pc
        self.reference_tilemask = tilemask

    @override
    def get_source_pointcloud(self) -> cwipc_pointcloud_wrapper:
        assert self._source_pointcloud
        return self._source_pointcloud
    
    @override
    def get_filtered_source_pointcloud(self) -> cwipc_pointcloud_wrapper:
        if self._filtered_source_pointcloud:
            return self._filtered_source_pointcloud
        return self.get_source_pointcloud()
    
    @override
    def get_reference_pointcloud(self) -> cwipc_pointcloud_wrapper:
        assert self._reference_pointcloud
        return self._reference_pointcloud
    
    @override
    def get_filtered_reference_pointcloud(self) -> cwipc_pointcloud_wrapper:
        if self._filtered_reference_pointcloud:
            return self._filtered_reference_pointcloud
        return self.get_reference_pointcloud()
    
    @override
    def apply_source_filter(self, filter : PointCloudFilter) -> None:
        self._filtered_source_pointcloud = filter(self.get_filtered_source_pointcloud())

    @override
    def apply_reference_filter(self, filter : PointCloudFilter) -> None:
        self._filtered_reference_pointcloud = filter(self.get_filtered_reference_pointcloud())

class BaseMulticamAlgorithm(MulticamAlgorithm):
    """Base class for most algorithms, both registration and alignment.

    Allows ading point clouds and inspecting them.
    """

    def __init__(self):
        self.per_camera_tilenum : List[int] = []
        self.original_pointcloud : Optional[cwipc_pointcloud_wrapper] = None
        self.verbose = False

    @override
    def set_tiled_pointcloud(self, pc : cwipc_pointcloud_wrapper) -> None:
        """Add each individual per-camera tile of this pointcloud, to be used during the algorithm run"""
        self.original_pointcloud = pc
        for tilemask in get_tiles_used(pc):
            self.per_camera_tilenum.append(tilemask)

    @override
    def tilemask_for_camera_index(self, cam_index : int) -> int:
        """Returns the tilenumber (used in the point cloud) for this index (used in the results)"""
        return self.per_camera_tilenum[cam_index]

    @override
    def camera_index_for_tilemask(self, tilenum : int) -> int:
        """Returns the  index (used in the results) for this tilenumber (used in the point cloud)"""
        for i in range(len(self.per_camera_tilenum)):
            if self.per_camera_tilenum[i] == tilenum:
                return i
        assert False, f"Tilenum {tilenum} not known"

    @override
    def camera_count(self):
        return len(self.per_camera_tilenum)

    def get_pc_for_tilemask(self, tilemask: int) -> cwipc_pointcloud_wrapper:
        """Get the pointcloud for a given camera number"""
        # xxxjack we can cache these.
        assert self.original_pointcloud
        pc = cwipc_tilefilter(self.original_pointcloud, tilemask)
        if not pc:
            raise ValueError(f"CamTilemaskera {tilemask} has no point cloud")
        return pc
        
    def get_pc_for_camnum(self, camnum: int) -> cwipc_pointcloud_wrapper:
        """Get the pointcloud for a given camera number"""
        # xxxjack implement this using get_pc_for_tilemask?
        assert self.original_pointcloud
        tilemask = self.tilemask_for_camera_index(camnum)
        pc = cwipc_tilefilter(self.original_pointcloud, tilemask)
        if not pc:
            raise ValueError(f"Camera {camnum} has no point cloud")
        return pc
