"""
The transform class.
"""
from __future__ import annotations

import enum
import numpy as np

from .vector import Vector


class Transform:
    """
    Transform class.
    """

    class Unit(enum.Enum):
        RADIANS = enum.auto()
        DEGREES = enum.auto()

    def __init__(self, initial_matrix: np.ndarray = np.identity(4, dtype=np.float32)) -> None:
        """
        Constructor.
        """

        self._matrix: np.ndarray = initial_matrix

    def __repr__(self) -> str:
        """
        Returns the string representation of this transform.

        :return: A string representation of this transform.
        """

        return f"Transform<{self._matrix}>"

    def __str__(self) -> str:
        """
        Returns the humanly-readable string representation of this transform.

        :return: A humanly-readable string representation of this transform.
        """

        return f"Transform<{self._matrix}>"

    @property
    def data(self) -> np.ndarray:
        """
        Returns the transformation matrix data.

        :return: The transformation matrix data.
        """

        return self._matrix

    def reset(self) -> None:
        """
        Reset the transform to identity.

        :return: None.
        """

        self._matrix = np.identity(4, dtype=np.float32)

    @staticmethod
    def from_translation(x: float, y: float, z: float) -> Transform:
        """
        Creates a translation transform.

        :param x: The x translation.
        :param y: The y translation.
        :param z: The z translation.
        :return: None.
        """

        matrix = np.identity(4, dtype=np.float32)
        matrix[0:3, 3] = [x, y, z]
        return Transform(matrix)

    @staticmethod
    def from_pitch_yaw_roll(pitch: float, yaw: float, roll: float, unit: Unit = Unit.RADIANS) -> Transform:
        """
        Creates a rotation transform from pitch-yaw-roll angles.

        :param pitch: The pitch angle.
        :param yaw: The yaw angle.
        :param roll: The roll angle.
        :param unit: The angle unit (either radians or degrees).
        :return: None.
        """

        # If the angles are given in degrees, convert them to radians
        if unit == Transform.Unit.DEGREES:
            pitch, yaw, roll = np.radians([pitch, yaw, roll])

        # Create individual rotation matrices
        roll_matrix = np.array([
            [1.0, 0.0, 0.0, 0.0],
            [0.0, np.cos(roll), -np.sin(roll), 0.0],
            [0.0, np.sin(roll), np.cos(roll), 0.0],
            [0.0, 0.0, 0.0, 1.0]
        ])

        pitch_matrix = np.array([
            [np.cos(pitch), 0.0, np.sin(pitch), 0.0],
            [0.0, 1, 0.0, 0.0],
            [-np.sin(pitch), 0.0, np.cos(pitch), 0.0],
            [0.0, 0.0, 0.0, 1.0]
        ])

        yaw_matrix = np.array([
            [np.cos(yaw), -np.sin(yaw), 0.0, 0.0],
            [np.sin(yaw), np.cos(yaw), 0.0, 0.0],
            [0.0, 0.0, 1.0, 0.0],
            [0.0, 0.0, 0.0, 1.0]
        ])

        # Store the joint matrix
        return Transform(yaw_matrix @ pitch_matrix @ roll_matrix)

    @staticmethod
    def from_perspective_projection(fov: float, aspect_ratio: float, near_plane: float, far_plane: float) -> Transform:
        """
        Creates an OpenGL-style perspective projection transform.

        :param fov: The field of view (in degrees.
        :param aspect_ratio: The aspect ratio.
        :param near_plane: The near clipping plane.
        :param far_plane: The far clipping plane.
        :return: The transform.
        """

        fov_radians = np.radians(fov)
        scaling_factor = 1.0 / np.tan(fov_radians / 2.0)

        # Initialize matrix
        matrix = np.zeros((4, 4), dtype=np.float32)

        # Apply standard OpenGL perspective formulas
        matrix[0, 0] = scaling_factor / aspect_ratio
        matrix[1, 1] = scaling_factor
        matrix[2, 2] = (far_plane + near_plane) / (near_plane - far_plane)
        matrix[2, 3] = (2.0 * far_plane * near_plane) / (near_plane - far_plane)
        matrix[3, 2] = -1.0

        return Transform(matrix)

    def apply(self, vector: Vector) -> Vector:
        """
        Applies the transform to a 3D vector.

        :param vector: The 3D vector.
        :return: The transformed vector.
        """

        v = np.dot(self._matrix, [vector.x, vector.y, vector.z, 1.0])
        return Vector(float(v[0]), float(v[1]), float(v[2]))

    @staticmethod
    def multiply(a: Transform, b: Transform) -> Transform:
        """
        Multiplies the two transforms (a * b).

        Note that the order of multiplication matters.

        :param a: The first transform.
        :param b: The second transform.
        :return: The combined transform.
        """

        return Transform(a.data @ b.data)

    def inverted(self) -> Transform:
        """
        Returns the inverse of the transform.

        :return: The inverted transform.
        """

        inverse = np.linalg.inv(self._matrix)
        return Transform(inverse)
