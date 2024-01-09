import open3d
import sys

def o3d_show_points(title : str, pc : open3d.geometry.PointCloud):
	"""Show a window with an open3d.geometry.PointCloud. """
	vis = open3d.visualization.Visualizer() # type: ignore
	vis.create_window(window_name=title)
	vis.add_geometry(pc)
	axes = open3d.geometry.TriangleMesh.create_coordinate_frame()
	vis.add_geometry(axes)
	viewControl = vis.get_view_control()
	pinholeCamera = viewControl.convert_to_pinhole_camera_parameters()
	pinholeCamera.extrinsic = [
	    [1, 0, 0, 0],
	    [0, 1, 0, 0],
	    [0, 0, 1, 0],
	    [0, 0, 0, 1]
	]
	viewControl.convert_from_pinhole_camera_parameters(pinholeCamera)
	vis.run()
	vis.destroy_window()

pcd = open3d.io.read_point_cloud(sys.argv[1])
o3d_show_points(open3d.__version__, pcd)
