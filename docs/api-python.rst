Python API
==========

The Python bindings are installed as the ``cwipc`` package.  A typical
installation via the macOS Homebrew formula or the Windows/Ubuntu installer
will generally **not** install the Python bindings into the system Python.

This is because nowadays Python packages are typically installed into virtual environments, and installing into the system Python can cause conflicts, or may not work at all. 
Instead, you should create a virtual environment and install the bindings there. Once you have activated your virtual environment, you can install the bindings using the `cwipc_pymodules_install.sh`, `cwipc_pymodules_install.bat` or `cwipc_pymodules_install.ps1` script.

Alternatively, from the command line you can use the ``cwipc python`` command, which will run the Python from the virtual environment that is bundled with the installation. This is a convenient way to run Python scripts that use the bindings without having to set up your own virtual environment.


To verify that the module is available::

    import cwipc
    print(cwipc.cwipc_get_version())

This will show the currently installed version of the cwipc package.

Example usage::

    import cwipc
    src = cwipc.cwipc_synthetic()
    src.start()
    for i in range(10):
        pc = src.get()
        print(f"Frame {i}: {pc.count()} points, timestamp {pc.timestamp()}")

The Python wrapper exposes most of the utility functions from C++ as
convenience helpers.  It is compatible with NumPy and PyOpenGL, making it
ideal for analysis or visualization in notebooks.

For the time being, please refer to the C++ API documentation for details on the most important classes and functions.

All the Python bindings are documented, so the Python mode of ``vscode`` and other development environments should be able to show you the available functions and their docstrings as you type.

Examples
--------

In a ``cwipc`` installation, you can find example Python scripts in the ``share/cwipc/python/examples`` directory. These cover a range of use cases, including capturing from cameras, playing back recorded point clouds, and using synthetic point cloud sources. You can run these examples directly with the ``cwipc python`` command, or copy them into your own projects as a starting point.

In a ``cwipc`` source distribution you can find these examples in ``cwipc_util/python/examples``.

Using from Jupyter
------------------

cwipc is usable from Jupyter notebooks. You can install the Python bindings into your Jupyter environment using the same installation scripts mentioned above. Once you have the bindings installed, you can import the ``cwipc`` module in your notebook and use it just like you would in a regular Python script.