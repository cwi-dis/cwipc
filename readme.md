# cwipc - CWI Point Clouds software suite


In order to facilitate working with point clouds as opaque objects - similar to how most software warks with images, or audio samples -  our group has developed an open source suite of libraries and tools that we call `cwipc` (abbreviation of CWI Point Clouds). The implementation builds on the PCL pointcloud library and various vendor-specific capturing libraries, but this is transparent to software using the `cwipc` suite (but it can access these representations if it needs to).

The idea behind a `cwipc` object is that it represents a point cloud (a collection of points with x/y/z coordinates, r/g/b values and possibly information on which camera angle the point was captured from, plus additional global information such as timestamp captured and voxel size, and optionally original RGB and D images or skeleton data). A cwipc object can be passed around without knowing what is inside it, and this can be done across implementation language boundaries while minimizing unnecessary memory copies. The library makes it possible to create end-to-end pipelines in order to capture, send, receive, and render dynamic point clouds. It is suitable for real-time applications and because point clouds can become very large special care is given to memory management and minimizing the amount of copying needed.

The core of the suite is `cwipc_util`, which handles the `cwipc` object implementation, its memory management and the multiple language bindings (C, C++, Python and C#). It also contains utility functions to read and write a `cwipc` object from a .ply file, apply different filters and transformations to the `cwipc` objects. In addition, it contains a set of tools (`cwipc_calibrate`) to align point clouds obtained from multiple cameras, a customized viewer (`cwipc_view`) to playback dynamic point clouds and a grabber tool (`cwipc_grab`) that allows you to grab point cloud frames from multiple devices or  from offline prerecorded files (which is what we used for creating the dataset).

The suite also contains modules `cwipc_kinect` and `cwipc_realsense2` that which capture point clouds from one or multiple cameras (Kinects and Realsense), and a module `cwipc_codec` that has the functionality to compress and decompress point clouds to make them suitable for real-time transmission. 

## More information

For now, refer to <https://www.dis.cwi.nl/cwipc-sxr-dataset/>.

## Build instructions

Check out the source repository from <https://github.com/cwi-dis/cwipc.git> and ensure you also check out the submodules and the git-lfs files. Use either

```
git clone https://github.com/cwi-dis/cwipc.git
cd cwipc
git submodule update --init --recursive
```

or

```
git clone --recurse-submodules https://github.com/cwi-dis/cwipc.git
```

Now you need to ensure that you have the required third party software installed (and, for Windows, that you have installed it in the expected location), see below.

Finally you build by using one of the build scripts:

- Linux:

  ```
  ./scripts/buildall.sh --sudo
  ```
  
  This will build everything, do a limited self-test and install into `/usr/local`.

- MacOS:

  ```
  ./scripts/buildall.sh
  ```
  
  This will build everything, do a limited self-test and install into `/usr/local`. Note that on the Mac _everything_ does not include the Kinect grabber: the Microsoft Kinect SDK is not yet available for MacOS.
  
  As of this writing some of the dependencies are not available yet for the M1 chip, but building for Rosetta 2 works.

- Windows:

  ```
  bash ./scripts/buildall-win.sh
  ```
  This will build everything, do a limited self-test and install into `./installed`.
  
Both these scripts configures, builds, tests and installs each of the submodules individually (in `build` directories under the submodule directory). If you need to tweak the build procedure, for example by adding `cmake` flags, you can use `rm -rf cwipc_*/build` to do a complete clean build.

The build scripts build the submodules in the correct order (`cwipc_util` first) and stop on error.

## Required third party software

On all platforms we require the standard development tools for the platform (XCode and the xcode command line tools on MacOS, Visual Studio 2019 on Windows, standard gcc tools on Linux).

On Linux the preferred way to install the third party software is through _apt_ or whatever package manager your platform supports. Our build process should then automatically pick everything up.

On MacOS the preferred way to install the third party software is through _Homebrew_ <https://brew.sh>. Our build process should then automatically pick everything up. The Python dependency deserves special note on MacOS, due to the many variants of Python available. If you ensure the `python3` command refers to the brew-installed Python 3.8 everything should go well. When building on an M1 Mac you should use the (Rosetta-based) Intel version of _Homebrew_, the one that installs into `/usr/local`, not into `/opt/homebrew`. Both versions can co-exist, ensure you have the Intel one in your path first when you build.

On Windows you have to install each package individually according to the instructions coming with the package. You need to ensure any "developer" option is installed. You also need to ensure that the folders with the DLLs and utilties for each package are on the `PATH` environment variable. For some packages you must install them in a specific place, or using specific settings.

Packages required:

- _cmake_. 
- _python 3_. Python 3.8 is preferred on Linux and MacOS, on Windows 3.7 may be better. On Windows you should install _"For All Users"_ and install into a writeable directory (such as `C:\Python37`).
- PCL 1.11 <https://pointclouds.org>. For Linux apt the package is called `libpcl-dev`. For MacOS brew you need to  `brew install pcl glfw3`. On Windows you need to ensure the DLL directories for PCL subpackages _OpenNI_ and _VTK_ are also on your `PATH`.
- _jpeg-turbo_ <https://libjpeg-turbo.org/>. On Mac you must force-link it if it conflicts with the normal `jpeg` brew package.
- _Intel Realsense SDK 2.0_ <https://github.com/IntelRealSense/librealsense>. 2.41 is known to work.
- _Azure Kinect SDK_ <https://github.com/microsoft/Azure-Kinect-Sensor-SDK>. 1.4.1 is known to work.
- _Azure Kinect Body Tracking SDK_. To be provided.

