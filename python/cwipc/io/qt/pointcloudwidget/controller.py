"""
A class for a first-person view controller.
"""
import numpy as np

from ..utility.transform import Transform
from ..utility.vector import Vector


class Controller:
    """
    Controller class.
    """

    def __init__(self) -> None:
        """
        Constructor.
        """

        self._rotation = Vector()  # Stores the pitch-yaw-roll angles in radians
        self._translation = Vector()  # Stores the XYZ translation

    @property
    def transform(self) -> Transform:
        """
        Returns the current transform.

        :return: The current transform.
        """

        rotation = Transform.from_pitch_yaw_roll(*tuple(self._rotation))
        translation = Transform.from_translation(*tuple(self._translation))
        return Transform.multiply(translation, rotation)

    @property
    def forward_vector(self) -> Vector:
        """
        Returns the forward vector.

        :return: The forward vector.
        """

        rotation = Transform.from_pitch_yaw_roll(*tuple(self._rotation))
        return Vector.normalize(rotation.apply(Vector(0.0, 0.0, 1.0)))

    @property
    def right_vector(self) -> Vector:
        """
        Returns the right vector.

        :return: The right vector.
        """

        rotation = Transform.from_pitch_yaw_roll(*tuple(self._rotation))
        return Vector.normalize(rotation.apply(Vector(1.0, 0.0, 0.0)))

    def reset(self) -> None:
        """
        Resets the controller to the default viewpoint.

        :return: None.
        """

        self._rotation = Vector()
        self._translation = Vector()

    def forward(self, speed: float) -> None:
        """
        Moves forward at the given speed.

        Use negative move speed for moving backwards.

        :param speed: The move speed.
        :return: None.
        """

        self._translation += self.forward_vector * speed

    def strafe(self, speed: float) -> None:
        """
        Strafes left or right at the given speed.

        :param speed: The speed.
        :return: None.
        """

        self._translation += self.right_vector * speed

    def rotate(self, pitch: float, yaw: float, roll: float) -> None:
        """
        Turns left or right at the given speed.

        :param pitch: The pitch angle (in degrees).
        :param yaw: The pitch angle (in degrees).
        :param roll: The pitch angle (in degrees).
        :return: None.
        """

        yaw, pitch, roll = np.radians(yaw), np.radians(pitch), np.radians(roll)
        self._rotation += Vector(yaw, pitch, roll)
