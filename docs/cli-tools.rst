Command‑line tools
==================

``cwipc`` provides a set of standalone command‑line utilities that can be used
without writing any code.  Until mid‑2024 there were several binaries; the
current entry point is the single ``cwipc`` program with subcommands (similar to
``git``).

Run ``cwipc --help`` to get a list of available commands.  Typical tools include:

* ``cwipc check`` – verify the installation and third‑party dependencies.
* ``cwipc register`` – assist with camera calibration and registration.
* ``cwipc grab`` – capture point clouds, convert between formats, compress/decompress.
* ``cwipc view`` – render a live or recorded point cloud stream.
* ``cwipc forward`` – stream point clouds over the network.

Each subcommand also supports ``--help`` for detailed usage information.  The
utilities are identical on all supported platforms and share a common set of
logging and configuration options.

Viewing example
---------------

The simplest viewing example is to run ``cwipc view --synthetic`` which renders a rotating synthetic point cloud.  
This is a good test of the installation and rendering capabilities. The synthetic point cloud
is approximately the size of a human being, and it has friendly blinking eyes so you can see
where its front is. This makes it helpful for debugging if you don't have a real camera
available.

If you run ``cwipc view`` without a source argument it will look for a ``cameraconfig.json`` in the current
directory and attempt to capture from these cameras. You will first have to setup your cameras,
see sections :doc:`hardware-setup` and :doc:`registration`.

While ``cwipc view`` is primarily intended for viewing point cloud streams (either from camera, pre-recorded or synthetic)
there is also ``cwipc play`` which is mainly intended for viewing single point clouds::

    cwipc play pointcloudfile.ply

Will show that point cloud and allow you to examine it. But note that ``cwipc play`` is really the same as ``cwipc view`` but
with the ``--paused`` option and a few other arguments.

Streaming example
-----------------

Start a server on port 4303::

    cwipc forward --synthetic --port 4303

Connect with a viewer on the same or another machine::

    cwipc view --netclient localhost:4303

This will "capture" the synthetic point cloud and stream it over the network to the viewer.  You can also use a real camera as the source for the server
The point cloud stream is compressed by default, but there are ``--noencode`` and ``--nodecode`` options to disable compression.

Streaming is single-sender to single-receiver, but there is a ``cwipc forward`` command that runs an ingest server
on port 4304 which will allow multiple clients to connect and receive the same live stream (by connecting to port 4303).

Conversion example
------------------

The `cwipc copy` command can be used to convert between point cloud formats, for example from a ply file to a cwipcdump file::

    cwipc copy pointcloudfile.ply pointcloudfile.cwipcdump

It can also apply filters and transformations. For example, to voxelize a pointcloud with a voxel size of 1cm use::

    cwipc copy --filter 'voxelize(0.01)' pointcloudfile.ply pointcloudfile_voxelized.ply

The `--help_filters` option will list the available filters and their parameters.

The `cwipc copy` command can handle sequences as well as single point cloud files. In this case you will often have to be more
precise in specifying what output format you want. See `--help` for the options.


Additional examples (recording, playback, synthetic streams) appear in the
README and in :doc:`raw-recording`.
