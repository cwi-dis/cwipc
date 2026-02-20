Recording point cloud streams
=============================

It is possible to record the raw camera streams (RGB and Depth) **while capturing point clouds**.
This recording is implemented in the low-level native ``cwipc`` capturing code, so it will work with
*any* application that uses ``cwipc``.

The recording is implemented in both the ``realsense`` and ``kinect`` capturers, and the recording can
subsequently be played back "as-live" using the ``realsense_playback`` or ``kinect_playback`` "capturers".

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

- Run your cwipc application as you would normally do. This could be ``cwipc_view`` or ``cwipc_forward``,
  but it could also be a Unity application for social XR, anything really.
- After the application has terminated examine the ``recording-20250505-1201`` subdirectory. It should have
  a number of ``.bag`` files (for Realsense) or ``.mkv`` files (for Kinect).
- Edit your ``cameraconfig.json`` again, and clear the ``record_to_directory`` field. It is important you
  do this **now**, otherwise the next time you run a ``cwipc`` application your recording will be overwritten.
- Copy your ``cameraconfig.json`` to ``recording-20250505-1201/cameraconfig.json``.
- Edit ``recording-20250505-1201/cameraconfig.json`` and make the following changes:

  - Set both the toplevel ``"type"`` and the ``"type"`` in each ``"camera"`` entry to ``"realsense_playback"``.
  - Or, ``"kinect_playback"`` for Kinect cameras.
  - Double-check that you have cleared ``record_to_directory``.

- Your recording is now complete. It consists of the complete directory ``recording-20250505-1201``,
  so the ``.bag`` files plus the modified ``cameraconfig.json``.

Testing your recording
----------------------

You can test the recording by simply running ``cwipc_view`` in the ``recording-20250505-1201`` directory.
You should see exactly the same point cloud sequence you recorded earlier.

Alternatively, you can play it back from another directory by running::

    cwipc_view --cameraconfig path/to/recording-20250505-1201/cameraconfig.json

Playing back your recording
----------------------------

The previous section has the relevant information. You use a ``cwipc`` tool with the ``--cameraconfig``
option or you use VR2Gather with the ``cameraconfig`` setting pointing to the correct path.

The playback will happen in "current realtime", so possibly frames will be dropped. You can pass the
``--nodrop`` argument to the cwipc utilities if you want to make sure every frame is processed
(at the expense of non-realtime playback). Use this for example what you want to convert the point
cloud stream to a compressed point cloud sequence.

One word of warning: for Realsense there is currently a problem with the ending of the recording.
This is not detected correctly, and the recording will try to loop, but do so in an unsynchronized way.
This will be fixed, the workaround currently is to watch what is happening and press control-C at the
end of the recording.
