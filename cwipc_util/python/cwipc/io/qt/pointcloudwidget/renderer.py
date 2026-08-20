"""
The Renderer class.
"""
from __future__ import annotations

import enum

from ctypes import c_void_p
from OpenGL.GL import *
from OpenGL.GL import shaders as gl_shaders

from .shaders import VERTEX_SHADER_SIMPLE, VERTEX_SHADER_SIZED_POINTS, FRAGMENT_SHADER

from ..utility.transform import Transform
from ..wrapper.pointcloud import PointCloud


class Renderer:
    """
    The Renderer class.
    """

    class Mode(enum.Enum):
        SIMPLE = enum.auto()
        SIZED_POINTS = enum.auto()

    def __init__(self, mode: Renderer.Mode = Mode.SIMPLE, clear_color: tuple[float, float, float] = (0.2, 0.2, 0.2)) -> None:
        """
        Constructor.

        :param mode: The rendering mode.
        """

        self._point_count: int = 0
        self._projection_transform: Transform = Transform()
        self._camera_transform: Transform = Transform()
        self._vertex_buffer: GLuint = 0
        self._vertex_array: GLuint = 0
        self._shader_programs: dict[Renderer.Mode, GLuint] = dict()
        self._mode: Renderer.Mode = mode
        self._clear_color: tuple[float, float, float] = clear_color
        self._point_size_factor: float = 10.0

    """
    Rendering.
    """

    def initialize(self) -> None:
        """
        Initialize the rendering.

        :return: None.
        """

        # Load shader programs
        simple_vertex_shader = gl_shaders.compileShader(VERTEX_SHADER_SIMPLE, GL_VERTEX_SHADER)
        sized_points_vertex_shader = gl_shaders.compileShader(VERTEX_SHADER_SIZED_POINTS, GL_VERTEX_SHADER)
        fragment_shader = gl_shaders.compileShader(FRAGMENT_SHADER, GL_FRAGMENT_SHADER)
        simple_program = gl_shaders.compileProgram(simple_vertex_shader, fragment_shader)
        sized_points_program = gl_shaders.compileProgram(sized_points_vertex_shader, fragment_shader)
        self._shader_programs = {Renderer.Mode.SIMPLE: simple_program, Renderer.Mode.SIZED_POINTS: sized_points_program}

        # Initialize internal buffers and arrays
        self._vertex_buffer = glGenBuffers(1)
        self._vertex_array = glGenVertexArrays(1)

        # Define constant OpenGL settings
        glClearColor(*self._clear_color, 1.0)
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL)
        glEnable(GL_DEPTH_TEST)
        glEnable(GL_PROGRAM_POINT_SIZE)

    def resize(self, width: int, height: int) -> None:
        """
        Handle viewpoint resizes.

        :return: None.
        """

        pass

    def render(self):
        """
        Render the content.

        :return: None.
        """

        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)

        assert self._mode in self._shader_programs, "Program mode not found."
        shader_program = self._shader_programs[self._mode]
        glUseProgram(shader_program)
        location = glGetUniformLocation(shader_program, "projection_matrix")
        glUniformMatrix4fv(location, 1, GL_TRUE, self._projection_transform.data)
        location = glGetUniformLocation(shader_program, "modelview_matrix")
        glUniformMatrix4fv(location, 1, GL_TRUE, self._camera_transform.data)
        location = glGetUniformLocation(shader_program, "point_size_factor")
        glUniform1f(location, self._point_size_factor)

        glBindVertexArray(self._vertex_array)
        glDrawArrays(GL_POINTS, 0, self._point_count)

        glBindVertexArray(0)
        glUseProgram(0)

    """
    Updates.
    """

    def update_data(self, point_cloud: PointCloud) -> None:
        """
        Updates the point cloud data.

        :param point_cloud: The point cloud.
        :return: None.
        """

        self._point_count = point_cloud.count

        # Update point cloud data in the vertex buffer
        if point_cloud.count > 0:
            glBindBuffer(GL_ARRAY_BUFFER, self._vertex_buffer)
            glBufferData(GL_ARRAY_BUFFER, point_cloud.points, GL_STATIC_DRAW)

            vertex_shift = 6 * 4
            glBindVertexArray(self._vertex_array)
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, vertex_shift, None)
            glEnableVertexAttribArray(0)
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, vertex_shift, c_void_p(12))
            glEnableVertexAttribArray(1)

        glBindVertexArray(0)
        glBindBuffer(GL_ARRAY_BUFFER, 0)

    def update_transform(self, projection_transform: Transform, camera_transform: Transform) -> None:
        """
        Updates the transformation.

        :param projection_transform: The projection transform.
        :param camera_transform: The camera transform.
        :return: None.
        """

        self._projection_transform = projection_transform
        self._camera_transform = camera_transform

    """
    Rendering mode.
    """

    def set_mode(self, mode: Renderer.Mode) -> None:
        """
        Sets the rendering mode.

        :param mode: The mode.
        :return: None.
        """

        self._mode = mode = mode

    @property
    def mode(self) -> Renderer.Mode:
        """
        Returns the current rendering mode.

        :return: The current rendering mode.
        """

        return self._mode

    """
    Point size.
    """

    def set_point_size_factor(self, point_size_factor: float) -> float:
        """
        Sets the point size factor.

        The point size factor cannot be smaller than 1.

        :param point_size_factor: The new point size factor.
        :return: The new value.
        """

        self._point_size_factor = max(1.0, point_size_factor)
        return self._point_size_factor

    @property
    def point_size_factor(self) -> float:
        """
        Returns the point size factor.

        :return: The point size factor.
        """

        return self._point_size_factor
