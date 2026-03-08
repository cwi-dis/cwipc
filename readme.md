# cwipc - CWI Point Clouds software suite

![build](https://github.com/cwi-dis/cwipc/actions/workflows/build.yml/badge.svg)
![docs](https://app.readthedocs.org/projects/cwipc/badge/?version=latest&style=flat)

## Overview

In order to facilitate working with point clouds as opaque objects - similar to how most software works with images, or audio samples -  our group has developed an open source suite of libraries and tools that we call `cwipc` (abbreviation of CWI Point Clouds). The implementation builds on the PCL pointcloud library and various vendor-specific capturing libraries, but this is transparent to software using the `cwipc` suite (but it can access these representations if it needs to).

The idea behind a `cwipc` object is that it represents a point cloud (a collection of points with x/y/z coordinates, r/g/b values and possibly information on which camera angle the point was captured from, plus additional global information such as timestamp captured and voxel size, and optionally original RGB and D images or skeleton data). A cwipc object can be passed around without knowing what is inside it, and this can be done across implementation language boundaries while minimizing unnecessary memory copies. The library makes it possible to create end-to-end pipelines in order to capture, send, receive, and render dynamic point clouds. It is suitable for real-time applications and because point clouds can become very large special care is given to memory management and minimizing the amount of copying needed.

The core of the suite is `cwipc_util`, which handles the `cwipc` object implementation, its memory management and the multiple language bindings (C, C++, Python and C#). It also contains utility functions to read and write a `cwipc` object from a .ply file, apply different filters and transformations to the `cwipc` objects. In addition, it contains a tool `cwipc register` to align point clouds obtained from multiple cameras, a customized viewer `cwipc view` to playback dynamic point clouds and a grabber tool `cwipc grab` that allows you to grab point cloud frames from multiple devices or  from offline prerecorded files (which is what we used for creating the dataset).

The suite also contains modules `cwipc_kinect`, `cwipc_orbbec` and `cwipc_realsense2` that which capture point clouds from one or multiple cameras (Kinects and Realsense), and a module `cwipc_codec` that has the functionality to compress and decompress point clouds to make them suitable for real-time transmission.

## Unity

There is a separate repository <https://github.com/cwi-dis/cwipc_unity> that contains the Unity package needed to use cwipc from Unity. See there for instructions. You will still need to install this package.

If you ended up on this web page because you got an error from Unity that pointed you here: you are using some Unity project that uses `cwipc_unity` but the native package (this one) has not been installed correctly. See the *Installation* link, below.

## Use cases

The use cases for `cwipc` that we foresee and try to support:

- Creating C or C++ programs that capture point cloud streams, compress them, and transmit them over the net. On Linux, Windows, MacOS or Android.
- Create Python programs with the same functionality.
- Create Python programs or Jupyter notebooks to do analysis of point clouds or point cloud streams, for example using `numpy` or `pyopen3d`.
- Create Unity applications that can capture, compress, transmit, decompress and render live point cloud streams.
- Allow some of the above functionality to be used without any programming at all, through command line programs.

## More information

For some background information, refer to <https://www.dis.cwi.nl/cwipc-sxr-dataset/>. If you use cwipc in a scientific
paper we would graciously accept a reference, please use the cwipc-sxr-dataset reference for the time being.

The change log can be found at [CHANGELOG.md](CHANGELOG.md).

The documentation can be read online via <https://cwipc.readthedocs.io>.
If you build cwipc from source you can also build it for reading it offline, in the `docs` folder.

### Documentation quick links:

- [Installation](https://cwipc.readthedocs.io/en/latest/installation.html)
- [Command line usage](https://cwipc.readthedocs.io/en/latest/cli-tools.html)



## Windows Installation troubleshooting

Windows installers often fail because each Windows computer is different. Moreover, cwipc depends on some third party packages (such as for Kinect support) that we cannot include in our installer because of licensing issues, so we have to rely on official installers for those packages.

After installing, run *Start menu* -> *cwipc* -> *Check cwipc installation*. This will open a CMD command window and try to find out if everything has been installed correctly. If there are any errors it may show a dialog which mentions which library has not been installed correctly. And there may be error messages in the output window.

If this shows any errors, try *Attempt to fix cwipc installation*. 

> As of July 2024 there is a problem you should check for first, which is not fixed by the automatic fixer.
> 
> You should go to *Apps* -> *Installed apps* and check that your version of *Microsoft Visual C++ 2015-2022 Redistributable (x64)* is at least version 14.40.33810.0. If your installed version is older: update. Searching for *MSVC redist* will find the download links.
> 
> This needs to be done because unfortunately Microsoft has made an incompatible change to their C++ Runtime, so any program built after about May 2024 will crash if it uses an older version of the runtime.

If after that the check command still fails, the problem is probably that one of the third party packages *is* installed on your computer, but it is an incorrect version, or it is installed in a different way than what *cwipc* expects. This will most likely be Python. Try uninstalling Python and letting the *cwipc* installer install the version it expects, in the way it expects.

> As is probably clear from this section, writing Windows installers is not our strong point. Please contact us if you can provide help.

### Debugging Windows installation issues

If all the tips above do not help you should try installing `ProcessMonitor` from `SysInternals` (Google will help you find it).
This tool is not for the faint of heart, but it allows you to trace each and every system call on your computer. So it *will* allow you to trace why weird things like "the cwipc utilities work but I cannot use the realsense capturer from within Unity" happen.
