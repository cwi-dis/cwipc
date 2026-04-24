"""
A Qt-based window that is a point cloud sink.
"""
import abc

from .qt.mainwindow import MainWindow
from .qt.pointcloudwidget.pointcloudwidget import PointCloudWidget
from .qt.wrapper.pointcloud import PointCloud

from ..net.abstract import cwipc_sink_abstract, cwipc_producer_abstract
from ..util import cwipc_pointcloud_wrapper


# todo: this workaround is required because the abstract sink class is based on ABC and not ABCMeta
class CombinedMeta(type(MainWindow), abc.ABCMeta):
    pass


class QtWindowSink(MainWindow, cwipc_sink_abstract, metaclass=CombinedMeta):
    """
    QtWindowSink class.
    """

    def __init__(self) -> None:
        """
        Constructor.

        :return: None.
        """

        super().__init__()

        # Create a point cloud widget
        self._point_cloud_widget: PointCloudWidget = PointCloudWidget(self)

        # Make it the central widget
        self.setCentralWidget(self._point_cloud_widget)

        # Set some default size
        self.resize(800, 600)

        # Show the window
        self.show()

    """
    Handle Qt events.
    """

    def process(self) -> None:
        """
        Processes Qt events.
        """

        self._application.processEvents()

    @property
    def available(self) -> bool:
        """
        Returns whether the sink window is available.

        :return: The availability.
        """

        return self.isVisible()

    """
    Abstract sink interface implementation.
    """

    def start(self) -> None:
        """
        Start the sink.

        :return: None.
        """

        pass

    def stop(self) -> None:
        """
        Stop the sink.

        Stops the sink by simply closing the window.

        :return: None.
        """

        self.hide()

    def set_producer(self, producer: cwipc_producer_abstract) -> None:
        """
        The rawsink will call producer.is_alive() to determine when it should stop transmitting.

        :param producer: The producer instance.
        :return: None.
        """

        # todo: does this have to store a producer instance as the Visualizer class does?
        pass

    def feed(self, point_cloud: cwipc_pointcloud_wrapper) -> None:
        """
        Receives a new point cloud.

        :param pc: The point cloud instance.
        :return: None.
        """

        # Wrap the point cloud instance and pass it to the point cloud widget
        wrapped_point_cloud = PointCloud(point_cloud)
        self._point_cloud_widget.set_point_cloud(wrapped_point_cloud)

    def statistics(self) -> None:
        """
        Prints statistics.

        todo: Mo statistics are implemented yet.

        :return: None.
        """

        pass
