# Setting up capture hardware

The two commonly supported depth camera families are **Azure Kinect / Kinect
for Azure** (including k4a and k4abt) and **Intel RealSense** (D4xx/Rs2).  This
section gives general advice; detailed calibration workflows are in
[registration.md](registration.md).

## General recommendations

* Use a stable mounting rig or tripod; avoid vibrations.
* Connect each camera to a dedicated USB 3.0 port / hub.
* Keep firmware and drivers up to date (see vendor documentation).

## Registration and synchronization

Multi‑camera setups require accurate extrinsic calibration and time
synchronization.  The `cwipc_register` tool walks you through capturing
chessboard patterns from each device and generating a `cameraconfig.json` file
containing the registration data.

For RealSense you typically run the SDK in *fisheye* mode and supply the
intrinsics to the registration tool.  For Kinect the body tracking SDK may
provide additional skeleton information.  Refer to the example configuration in
`doc/registration.md`.

## Playback of recordings

Both capturers support recording of raw camera streams in parallel with
point‑cloud capture.  See [Recording point cloud streams](raw-recording.md) for
instructions on creating and playing back these recordings.

> Note: recordings are heavy on I/O; use a fast SSD especially for multi‑camera
> systems.
