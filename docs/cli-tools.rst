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
* ``cwipc copy`` – copy point clouds between various storage formats and types.
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
see section :doc:`registration`.

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

Streaming is single-sender to single-receiver, but there is a ``cwipc netserver`` command that runs an ingest server
on port 4304 which will allow multiple clients to connect and receive the same live stream (by connecting to port 4303).
This also allows you yo stream through NAT firewalls, as long as the forwarding server is on a machine with a public IP address.

On the server (let's say ``myserver.example.com``), run::

    cwipc netserver

On the sending client, run::

    cwipc forward --synthetic --forward  myserver.example.com:4303

On the receiving clients, run::

    cwipc view --netclient myserver.example.com:4303

Conversion example
------------------

The `cwipc copy` command can be used to convert between point cloud formats, for example from a ply file to a cwipcdump file::

    cwipc copy pointcloudfile.ply pointcloudfile.cwipcdump

It can also apply filters and transformations. For example, to voxelize a pointcloud with a voxel size of 1cm use::

    cwipc copy --filter 'voxelize(0.01)' pointcloudfile.ply pointcloudfile_voxelized.ply

The `--help_filters` option will list the available filters and their parameters.

The `cwipc copy` command can handle sequences as well as single point cloud files. In this case you will often have to be more
precise in specifying what output format you want. See `--help` for the options.


Additional examples (recording, playback, synthetic streams) appear in :doc:`raw-recording`.

Common arguments
----------------

Most ``cwipc`` subcommands share a common set of arguments for controlling input sources,
logging, and debugging. Use ``cwipc <subcommand> --help`` to see the full list of available options for each command.  Most common arguments are listed below.

Global arguments
^^^^^^^^^^^^^^^^

* ``--version`` – print version and exit.
* ``-v``, ``--verbose`` – print information about each point cloud while it is processed.
  Use ``-vv`` for even more verbosity.
* ``--logging LEVEL`` – set cwipc logging level (``error``, ``warning``, ``info``, ``debug``)
  and capture log messages.
* ``--pausefordebug`` – pause at begin and end of run to allow attaching a debugger or profiler.
* ``--debugpy`` – pause at begin of run to wait for debugpy to attach (Python debugging).

Input source selection
^^^^^^^^^^^^^^^^^^^^^^

These arguments are mutually exclusive; you can only specify one input source. If none is specified,
the tool will look for a ``cameraconfig.json`` in the current directory:

* ``--cameraconfig PATH`` – specify camera configuration file (default: ``./cameraconfig.json``).
  Use ``auto`` to detect any attached camera without configuration.
* ``--kinect`` – use Azure Kinect capturer.
* ``--realsense`` – use Intel RealSense capturer.
* ``--orbbec`` – use Orbbec Femto capturer.
* ``--synthetic`` – use synthetic point cloud source (e.g. a rotating virtual human).
* ``--proxy PORT`` – use proxy server source that listens on ``PORT``.
* ``--netclient HOST:PORT`` – receive (compressed) point clouds from network server on ``HOST:PORT``.
* ``--lldplay URL`` – use DASH (compressed) point cloud stream from ``URL``.
* ``--mt-netclient HOST:PORT:NT:NQ`` – multi-tile, multi-quality version of ``--netclient``.
* ``--mt-lldplay URL`` – multi-tile, multi-quality version of ``--lldplay``.
* ``--playback PATH`` – use point cloud(s) from PLY or cwipcdump file or directory (alphabetical order).

Input arguments
^^^^^^^^^^^^^^^

These arguments control how the input source behaves:

* ``--nodecode`` – receive uncompressed point clouds with ``--netclient`` and ``--lldplay``
  (default: compressed with cwipc_codec).
* ``--loop`` – with ``--playback``, loop the contents instead of terminating after the last file.
* ``--npoints N`` – limit number of points (approximately) in synthetic point cloud.
* ``--fps N`` – limit playback or capture rate to ``N`` frames per second (for some capturers).
* ``--retimestamp`` – set timestamps to wall clock instead of recorded timestamps.
* ``--count N`` – stop after receiving ``N`` point clouds.
* ``--inpoint N`` – start at frame with timestamp ``> N``.
* ``--outpoint N`` – stop at frame with timestamp ``>= N``.

Environment variables
^^^^^^^^^^^^^^^^^^^^^

Several environment variables control cwipc behavior at runtime. Most of these are handled by the low-level
cwipc API, so they will also work when using the language bindings or developing your own applications.

Common environment variables include:

* ``CWIPC_LOGGING`` controls logging output level and destination. Can be set to a log level
  (e.g. ``TRACE``, ``DEBUG``, ``INFO``, ``WARNING``, ``ERROR``) or combined with a file path
  (e.g. ``CWIPC_LOGGING=DEBUG:path/to/output/file.txt``). Defaults to warnings and errors to stderr.
* ``CWIPC_LIBRARY_DIR`` (Windows only) set to the directory containing cwipc runtime DLLs and dependencies
  to help Python packages locate the installation. On macOS and Linux this is typically handled by ``rpath``.
* ``_CWIPC_DEBUG_DLL_SEARCH_PATH`` set to ``1`` or ``true`` to print debug output when Python packages
  attempt to locate the cwipc installation.
* ``EDITOR`` used by tools that allow editing configuration files (e.g. ``cwipc register``).
* ``SIGNALS_SMD_PATH`` (low-latency DASH only) path to the ``lldash`` runtime DLLs.
* ``LLDASH_LOGGING`` set to enable more verbose logging from the ``lldash`` component.
