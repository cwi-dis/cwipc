C# / Unity API
==============

C# support is provided primarily for Unity applications.  The managed
assembly is distributed via the separate `cwipc_unity
<https://github.com/cwi-dis/cwipc_unity>`_ repository and wraps the native ``cwipc``
libraries.

From Unity, import the ``cwipc_unity`` package and use the supplied prefabs and
scripts such as ``cwipc_playback_stream`` or ``StreamedPointCloudReader``.

Standalone .NET or Mono programs can also P/Invoke the native ``cwipc`` DLLs
located in the installation, although this is less common.

Refer to the Unity repository for detailed examples and documentation.
