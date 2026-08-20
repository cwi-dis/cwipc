"""
A Qt OpenGLWidget which displays point clouds and allows for user exploration.
"""
import time
import traceback

from dataclasses import dataclass
from PySide6.QtCore import Qt, QPointF, QTimer
from PySide6.QtGui import QSurfaceFormat, QKeyEvent, QMouseEvent, QWheelEvent
from PySide6.QtOpenGLWidgets import QOpenGLWidget
from PySide6.QtWidgets import QWidget
from typing import Optional

from .controller import Controller
from .renderer import Renderer

from ..utility.transform import Transform
from ..wrapper.pointcloud import PointCloud


class PointCloudWidget(QOpenGLWidget):
    """
    PointCloudWidget class.
    """

    @dataclass(frozen=True)
    class Configuration:
        """
        The widget configuration.
        """

        # Widget settings
        fps: int = 30
        # Virtual camera settings
        field_of_view: float = 45.0
        near_plane: float = 0.0001
        far_plane: float = 100.0
        # Controller settings
        allow_control: bool = True
        move_speed: float = 5.0  # Given in units per second
        mouse_sensitivity: float = 0.2
        wheel_sensitivity: float = 0.01

    def __init__(self, parent: QWidget, configuration: Configuration = Configuration()) -> None:
        """
        Constructor.

        :param parent: The parent widget.
        :param configuration: The widget configuration.
        """

        super().__init__(parent)

        # (1) Store the user-configuration
        self._configuration: PointCloudWidget.Configuration = configuration

        # (2) Setup surface format
        surface_format = QSurfaceFormat.defaultFormat()
        surface_format.setVersion(4, 3)
        surface_format.setProfile(QSurfaceFormat.OpenGLContextProfile.CoreProfile)
        surface_format.setOption(QSurfaceFormat.FormatOption.DebugContext)  # Toggle debugging off for more performance
        surface_format.setSwapBehavior(QSurfaceFormat.SwapBehavior.DoubleBuffer)
        surface_format.setRedBufferSize(8)
        surface_format.setGreenBufferSize(8)
        surface_format.setBlueBufferSize(8)
        surface_format.setAlphaBufferSize(8)
        surface_format.setDepthBufferSize(24)
        surface_format.setStencilBufferSize(0)
        surface_format.setSamples(16)
        surface_format.setSwapInterval(0)
        self.setFormat(surface_format)

        # (3) Create the renderer instance and internal point cloud reference
        self._renderer: Renderer = Renderer()
        self._point_cloud: Optional[PointCloud] = None

        # (4) Create controller and auxiliary control internals
        self._controller: Controller = Controller()
        self._key_map: set[int] = set()
        self._mouse_click_position: QPointF = QPointF()
        self._projection_transform: Transform = Transform()

        # (5) Set strong focus and enable mouse tracking for the widget to always receive keyboard and mouse inputs
        if configuration.allow_control:
            self.setFocusPolicy(Qt.FocusPolicy.StrongFocus)
            self.setMouseTracking(True)

        # (6) Set up a timer that triggers the internal "game loop" at a regular interval
        self._timer = QTimer()
        self._last_time: int = time.perf_counter_ns()
        self._timer.timeout.connect(self._tick)
        self._timer.start(int(1000.0 / float(configuration.fps)))

    """
    Rendering.
    """

    def initializeGL(self) -> None:
        """
        Initialize the OpenGL widget.

        :return: None.
        """

        super().initializeGL()

        # Initialize with special exception handling because initializeGL() swallows exceptions
        try:
            self._renderer.initialize()
        except Exception as e:
            traceback.print_exc()
            exit(1)

    def resizeGL(self, width: int, height: int) -> None:
        """
        Handles resizing of the widget.

        :param width: The new widget width.
        :param height: The new width height.
        :return: None.
        """

        self._projection_transform = Transform.from_perspective_projection(
            self._configuration.field_of_view,
            width / height if height != 0 else 1.0,
            self._configuration.near_plane,
            self._configuration.far_plane
        )

    def paintGL(self) -> None:
        """
        Paint the OpenGL widget.

        This method also performs the delayed upload of the point cloud data as it is the only
        point where a valid OpenGL context is safely guaranteed.

        :return: None.
        """

        super().paintGL()

        # Upload pointcloud data if needed
        if self._point_cloud:
            self._renderer.update_data(self._point_cloud)
            self._point_cloud = None

        # Update transforms
        camera_transform = self._controller.transform.inverted()
        self._renderer.update_transform(self._projection_transform, camera_transform)

        # Render the point cloud
        self._renderer.render()

    def set_point_cloud(self, point_cloud: PointCloud) -> None:
        """
        Updates the point cloud of the renderer.

        :param point_cloud: The new point cloud.
        :return: None.
        """

        self._point_cloud = point_cloud
        self.update()

    """
    User-control.
    """

    def keyPressEvent(self, event: QKeyEvent) -> None:
        """
        Handle key press events.

        :param event: The keyboard event.
        :return: None.
        """

        if self._configuration.allow_control:
            key = event.key()
            accepted_keymap_keys = {
                Qt.Key.Key_W, Qt.Key.Key_A, Qt.Key.Key_S, Qt.Key.Key_D,
                Qt.Key.Key_Up, Qt.Key.Key_Left, Qt.Key.Key_Down, Qt.Key.Key_Right,
                Qt.Key.Key_Space
            }
            if key in accepted_keymap_keys:
                self._key_map.add(key)
                event.accept()

            if key == Qt.Key.Key_1:
                self._renderer.set_mode(Renderer.Mode.SIMPLE)
            if key == Qt.Key.Key_2:
                self._renderer.set_mode(Renderer.Mode.SIZED_POINTS)
            if key == Qt.Key.Key_Plus:
                self._renderer.set_point_size_factor(self._renderer.point_size_factor + 1.0)
            if key == Qt.Key.Key_Minus:
                self._renderer.set_point_size_factor(self._renderer.point_size_factor - 1.0)

        super().keyPressEvent(event)

    def keyReleaseEvent(self, event: QKeyEvent) -> None:
        """
        Handle key release events.

        :param event: The keyboard event.
        :return: None.
        """

        if self._configuration.allow_control:
            key = event.key()
            if key in self._key_map:
                self._key_map.remove(key)
            event.accept()

        super().keyReleaseEvent(event)

    def mousePressEvent(self, event: QMouseEvent) -> None:
        """
        Handles mouse press events.

        :param event: The event.
        :return: None.
        """

        super().mousePressEvent(event)

        # Simply store the local position of the click
        self._mouse_click_position = event.localPos()

    def mouseMoveEvent(self, event: QMouseEvent) -> None:
        """
        Handles mouse movement events.

        :param event: The event.
        :return: None.
        """

        if self._configuration.allow_control:
            if event.buttons() == Qt.MouseButton.LeftButton:
                difference = self._mouse_click_position - event.localPos()
                self._mouse_click_position = event.localPos()
                yaw = difference.x() * self._configuration.mouse_sensitivity
                roll = difference.y() * self._configuration.mouse_sensitivity
                self._controller.rotate(0.0, yaw, roll)
                event.accept()

        super().mouseMoveEvent(event)

    def wheelEvent(self, event: QWheelEvent) -> None:
        """
        Handles mouse wheel events.

        :param event: The event.
        :return: None.
        """

        if self._configuration.allow_control:
            delta = event.angleDelta()
            if delta.y() != 0.0:
                self._controller.forward(-delta.y() * self._configuration.wheel_sensitivity)
                event.accept()

        super().wheelEvent(event)

    """
    Internals.
    """

    def _tick(self) -> None:
        """
        Internal tick method.

        :return: None.
        """

        # Track elapsed time since last tick call
        current_time = time.perf_counter_ns()
        elapsed_seconds = (current_time - self._last_time) / (10 ** 9)
        self._last_time = current_time

        # Handle key inputs
        if self._configuration.allow_control:
            # Compute time-based movement speed
            move_speed = elapsed_seconds * self._configuration.move_speed

            # Handle keys
            if {Qt.Key.Key_W, Qt.Key.Key_Up} & self._key_map:
                self._controller.forward(-move_speed)
            if {Qt.Key.Key_A, Qt.Key.Key_Left} & self._key_map:
                self._controller.strafe(-move_speed)
            if {Qt.Key.Key_S, Qt.Key.Key_Down} & self._key_map:
                self._controller.forward(move_speed)
            if {Qt.Key.Key_D, Qt.Key.Key_Right} & self._key_map:
                self._controller.strafe(move_speed)
            if Qt.Key.Key_Space in self._key_map:
                self._controller.reset()

        # Force repaint
        self.update()
