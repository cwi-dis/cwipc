Setting up capture hardware
============================

The commonly supported depth camera families are **Azure Kinect / Kinect for Azure** (including k4a and k4abt),
**Intel RealSense** (D4xx/Rs2), and **Orbbec Femto** series cameras.  This section gives general advice; detailed calibration workflows are in :doc:`registration`.

General recommendations
-----------------------

* Use a stable mounting rig or tripod; avoid vibrations.
* Connect each camera to a dedicated USB 3.0 port / hub.
* Keep firmware and drivers up to date (see vendor documentation).

.. note::
   Detailed hardware setup guides with photos (cable diagrams, tripod configurations, multi-camera rigs, etc.)
   can be added here. Please contribute your setup pictures and instructions if you have working configurations
   to share.

Registration and synchronization
---------------------------------

Multi‑camera setups require accurate extrinsic calibration and time
synchronization.  The ``cwipc_register`` tool walks you through capturing
chessboard patterns from each device and generating a ``cameraconfig.json`` file
containing the registration data.

For RealSense you typically run the SDK in *fisheye* mode and supply the
intrinsics to the registration tool.  For Kinect the body tracking SDK may
provide additional skeleton information. For Orbbec cameras, firmware and SDK
versions should be kept up to date for optimal performance.  Refer to the example configuration in :doc:`registration`.

Playback of recordings
----------------------

Both capturers support recording of raw camera streams in parallel with
point‑cloud capture.  See :doc:`raw-recording` for
instructions on creating and playing back these recordings.

.. note::
   Recordings are heavy on I/O; use a fast SSD especially for multi‑camera
   systems.
