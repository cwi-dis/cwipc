Building from source
====================

This section describes how to compile **cwipc** from the GitHub repository on
macOS, Linux or Windows.  The project uses CMake with a vcpkg‑based third
dependency tree.

If you want to build offline (i.e. without cloning the repository) you must download the ``cwipc--fullsource`` archive,
either as ``.zip`` or ``.tar.gz``, from the `releases page <https://github.com/cwi-dis/cwipc/releases>`_. The standard GitHub ``Source code`` downloads do not include the submodules therefore they will not work.

Prerequisites
-------------

* A recent CMake (3.20+).
* A C++17 compiler (clang, gcc, MSVC).
* Git toolchain.
* Python 3.12.
* ``bash`` or ``zsh`` on macOS/Linux; PowerShell on Windows.

Building from the command line
------------------------------

1. Clone the repository, ensuring you also pull the submodules and the git lfs content::

       git clone https://github.com/cwi-dis/cwipc.git
       cd cwipc
       git submodule update --init --recursive
       git lfs init
       git lfs pull

2. Install third‑party packages:
   
   * macOS: ``scripts/install-3rdparty-macos.sh``
   * Ubuntu: ``scripts/install-3rdparty-ubuntu2404.sh``
   * Windows: ``scripts/install-3rdparty-windows.ps1`` (run as Administrator).

3. Configure and build with CMake presets::

       cmake --list-presets          # This shows the available presets on your current platform.
       cmake --preset mac-production # or another one that is listed

   The presets automatically supply the vcpkg toolchain.

4. Build the code (in the ``build`` subdirectory)::

       cmake --build --preset mac-production

5. (Optional) Run the unit tests::

       ctest --preset mac-production

6. Install to a local prefix::

       cmake --install build --prefix ../cwipc-local-install

   The default prefix is ``/usr/local`` on Unix and ``C:\Program Files\cwipc`` on
   Windows; override with ``-DCMAKE_INSTALL_PREFIX`` when invoking ``cmake``.

7. Try it:

   On Windows you have to add the installed ``bin`` directory to your ``PATH`` environment variable.
   On Mac and Linux this is optional (because the programs use *rpath* to find the shared libraries).

Building from vscode
--------------------


Easiest is to do all development from *Visual Studio Code*, *vscode* for short. Get that. You also want to get your standard C/C++ compilers and such, ``cmake`` and *Python* (3.12 preferred).

On Windows use the *Visual Studio Community Installer* to get the compilers and cmake. Download Python and vscode yourself. Note:

- you should install Python *"For All Users"*. 
- You should install into a writeable directory, such as ``C:/Python39`` otherwise you will have to use *Run as Administrator* for various build steps.

On Linux use the system package manager for everything, except you may have to install vscode differently, use google to check.

On Mac use the *XCode* installer for the compilers, *brew* for cmake, and google for vscode.

For Android you will have to install the correct Android crosscompilers and all that.

When you have vscode working you want to install the following extensions for it:

- C/C++ Tools and Extension Pack
- CMake and CMake Tools
- Python, Pylance and Python Debugger

Debugging
---------

The cmake presets have a `develop` option for each platform. When you build these the `vscode` debuggers will work (both the native C/C++ debuggers and of course also the Python debugger).

After you have built cwipc, you should run (Mac, Linux)::

       source scripts/activate

Or, on Windows::

       & scripts\activate.ps1


This will ensure you have the right ``build/bin`` directory on your ``PATH``, and the right dynamic library search path, and the right Python ``venv`` activated with all the *cwipc* commands and modules available. 


Some issues can then be debugged with the C or C++ command line utilities (by putting breakpoints at the right location and running them with the correct command line arguments).

Many issues are easier to debug with the Python scripts. There are some hooks in place to help with this.

All Python scripts accept a ``--debugpy`` command line option. This will pause the script at begin of run and you can attach the vscode Python debugger to the process.

All Python scripts also accept a ``--pausefordebug`` command line option. This will pause the script at begin of run (and end of run), waiting for you to press ``Y``. While the script is paused you can obtain the process ID and attach the vscode C/C++ debugger to the process.


Additionally, you can send ``SIGQUIT`` to all the Python scripts to cause them to dump the Python stacktraces of all threads.

Notes
-----

* Python bindings are built as part of the main build, and the wheels are saved in `share/cwipc/python`.
  The ``cwipc_pymodules_install.sh`` (or ``cwipc_pymodules_install.ps1``) script can be used to install them.
* To build Android targets, use the corresponding preset ``android-release``.

Environment variables
---------------------

- ``CWIPC_VERSION`` can set the version string.
- ``CWIPC_TEST_HAVE_KINECT_HARDWARE`` can be set to run the tests that require Azure Kinect hardware.
- ``CWIPC_TEST_HAVE_ORBBEC_HARDWARE`` can be set to run the tests that require Orbbec hardware.
- ``CWIPC_TEST_HAVE_REALSENSE2_HARDWARE`` can be set to run the tests that require Realsense2 hardware.
