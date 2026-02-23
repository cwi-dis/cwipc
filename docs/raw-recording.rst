Recording point cloud streams
=============================

It is possible to record the raw camera streams (RGB and Depth) **while capturing point clouds**.
This recording is implemented in the low-level native ``cwipc`` capturing code, so it will work with
*any* application that uses ``cwipc``.

The recording is implemented in all of the ``realsense2``, ``orbbec`` and ``kinect`` capturers, and the recording can
subsequently be played back "as-live" using the ``realsense_playback``, ``orbbec_playback`` or ``kinect_playback`` "capturers".

Recording also works for multi-camera setup, and synchronization between cameras should be maintained.

Recording is not very CPU-intensive, but it **is** very heavy on I/O. You need a fast SSD, especially
for multi-camera recordings.

Recording
---------

It is assumed you have a ``cameraconfig.json`` that has the correct camera registration, synchronization
and hardware parameters.

- Create a subdirectory where the recording will be saved, let's say ``recording-20250505-1201``.
- Edit ``cameraconfig.json`` and in the ``"system"`` section add a field::

    "record_to_directory" : "recording-20250505-1201"
- You may also wat to look at the ``"fps"`` field, and the width and height of the RGB and Depth streams, and adjust them to your needs. The higher the resolution and fps, the more disk space you will need for the recording.
- Run your cwipc application as you would normally do. This could be ``cwipc view`` or ``cwipc forward``,
  but it could also be a Unity application for social XR, anything really.
- After the application has terminated examine the ``recording-20250505-1201`` subdirectory. It should have
  a number of ``.bag`` files (for Realsense) or ``.mkv`` files (for Kinect or Orbbec).
  It will also have a new ``cameraconfig.json`` which is a copy of the one you used for recording, but with some modifications to make it suitable for playback. 
- Edit your ``cameraconfig.json`` again, and clear the ``record_to_directory`` field. It is important you
  do this **now**, otherwise the next time you run a ``cwipc`` application your recording will be overwritten.

- Your recording is now complete. It consists of the complete directory ``recording-20250505-1201``,
  so the ``.bag`` files plus the modified ``cameraconfig.json``.

Testing your recording
----------------------

You can test the recording by simply running ``cwipc view`` in the ``recording-20250505-1201`` directory.
You should see exactly the same point cloud sequence you recorded earlier.

Alternatively, you can play it back from another directory by running::

    cwipc view --cameraconfig path/to/recording-20250505-1201/cameraconfig.json

or::

    cwipc play path/to/recording-20250505-1201

Playing back your recording
----------------------------

The previous section has the relevant information. You use a ``cwipc`` tool with the ``--cameraconfig``
option or you use VR2Gather with the ``cameraconfig`` setting pointing to the correct path.

The playback will happen in "current realtime", so possibly frames will be dropped. You can pass the
``--nodrop`` argument to the cwipc utilities if you want to make sure every frame is processed
(at the expense of non-realtime playback). Use this for example what you want to convert the point
cloud stream to a compressed point cloud sequence.

