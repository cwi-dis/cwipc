"""
The vector class.
"""
from __future__ import annotations

import numpy as np

from dataclasses import astuple, dataclass
from typing import Iterator


@dataclass(frozen=True)
class Vector:
    x: float = 0.0
    y: float = 0.0
    z: float = 0.0

    @staticmethod
    def normalize(v: Vector) -> Vector:
        """
        Returns the normalized input vector.

        :return: The normalized input vector.
        """

        a = np.array([v.x, v.y, v.z])
        norm = np.linalg.norm(a)
        normalized = a / norm if norm > 0 else a
        return Vector(*normalized.tolist())

    def __add__(self, other: Vector) -> Vector:
        """
        Implements the addition of two vectors.

        :param other: The other vector.
        :return: The sum of the vectors.
        """

        return Vector(self.x + other.x, self.y + other.y, self.z + other.z)

    def __mul__(self, other: float | Vector) -> Vector:
        """
        Implements component-wise multiplication with a scalar of another vector.

        :param other: A scalar value or another vector.
        :return: The resulting vector.
        """

        assert isinstance(other, (float, Vector)), "Invalid factor"

        if isinstance(other, float):
            multiplier = Vector(other, other, other)
        else:
            multiplier = other

        return Vector(self.x * multiplier.x, self.y * multiplier.y, self.z * multiplier.z)

    def __iter__(self) -> Iterator[float]:
        """
        Returns an iterator over the class for convenience.

        :return:
        """

        return iter(astuple(self))
