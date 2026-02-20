Python API
==========

The Python bindings are installed as the ``cwipc`` package.  A typical
installation via the macOS Homebrew formula or the Windows/Ubuntu installer
will place the package in the active Python environment.

To verify that the module is available::

    import cwipc
    print(cwipc.__version__)

Example usage::

    import cwipc

    g = cwipc.grabber()
    g.open("--synthetic")
    for pkt in g:
        print(pkt.points.shape)  # numpy array of points

The Python wrapper exposes most of the utility functions from C++ as
convenience helpers.  It is compatible with NumPy and PyOpenGL, making it
ideal for analysis or visualization in notebooks.

Binding generation is handled by the ``cwipc_util/src/python/`` code; see the
``cwipc_pymodules_install.sh`` script for installation details.
