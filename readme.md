# cwipc - CWI Point Clouds software suite

![build](https://github.com/cwi-dis/cwipc/actions/workflows/build.yml/badge.svg)

In order to facilitate working with point clouds as opaque objects - similar to how most software works with images, or audio samples -  our group has developed an open source suite of libraries and tools that we call `cwipc` (abbreviation of CWI Point Clouds). The implementation builds on the PCL pointcloud library and various vendor-specific capturing libraries, but this is transparent to software using the `cwipc` suite (but it can access these representations if it needs to).

The idea behind a `cwipc` object is that it represents a point cloud (a collection of points with x/y/z coordinates, r/g/b values and possibly information on which camera angle the point was captured from, plus additional global information such as timestamp captured and voxel size, and optionally original RGB and D images or skeleton data). A cwipc object can be passed around without knowing what is inside it, and this can be done across implementation language boundaries while minimizing unnecessary memory copies. The library makes it possible to create end-to-end pipelines in order to capture, send, receive, and render dynamic point clouds. It is suitable for real-time applications and because point clouds can become very large special care is given to memory management and minimizing the amount of copying needed.

The core of the suite is `cwipc_util`, which handles the `cwipc` object implementation, its memory management and the multiple language bindings (C, C++, Python and C#). It also contains utility functions to read and write a `cwipc` object from a .ply file, apply different filters and transformations to the `cwipc` objects. In addition, it contains a set of tools (`cwipc_calibrate`) to align point clouds obtained from multiple cameras, a customized viewer (`cwipc_view`) to playback dynamic point clouds and a grabber tool (`cwipc_grab`) that allows you to grab point cloud frames from multiple devices or  from offline prerecorded files (which is what we used for creating the dataset).

The suite also contains modules `cwipc_kinect` and `cwipc_realsense2` that which capture point clouds from one or multiple cameras (Kinects and Realsense), and a module `cwipc_codec` that has the functionality to compress and decompress point clouds to make them suitable for real-time transmission. 

## More information

For now, refer to <https://www.dis.cwi.nl/cwipc-sxr-dataset/>.

Documentation on the API can be created using _Doxygen_ in `cwipc_util/doc`, and will be made available here at some point in the future.

The change log can be found at [CHANGELOG.md](CHANGELOG.md).

## Installation

The simplest way to install cwipc is through a prebuilt installer. This will install everything in the standard location, and it allows running the command line tools as well as developing C, C++, Python or Unity programs that use the cwipc library.

After installation, run `cwipc_view --synthetic` from a shell (terminal window, command prompt). It should show you a window with a rotating synthetic point cloud if everything is installed correctly. There is also a command line utility `cwipc_check` that will test that all third-party requirements have been installed correctly. Especially on Windows this can be very helpful (see below).

See below if you want to install to a different location, or if you want to modify cwipc itself and build it from source.

### Windows

Download the windows installer `.exe` for the most recent cwipc release from <https://github.com/cwi-dis/cwipc/releases/latest>.

Run it, and it will install the cwipc command line tools and the C++ and Python APIs.

It will _also install all required third party packages_, unless a usable version is detected.

### Windows - check installation

Windows installers often fail because each Windows computer is different. Moreover, cwipc depends on a number of third party packages (such as the Realsense and Kinect support) that we cannot include in our installer because of licensing issues, so we have to rely on official installers for those packages.

After installing, run _Start menu_ -> _cwipc_ -> _Check cwipc installation_. This will open a CMD command window and try to find out if everything has been installed correctly. If there are any errors it may show a dialog which mentions which library has not been installed correctly. And there may be error messages in the output window.

If this shows any errors, try _Attempt to fix cwipc installation_. 

If after that the check command still fails, the problem is probably that one of the third party packages _is_ installed on your computer, but it is an incorrect version, or it is installed in a different way than what _cwipc_ expects.

Try to determine which package is responsible for the failure, and uninstall it. Then reboot and re-try the _fix cwipc installation_. This _should_ install the correct version of every package, and install it with the expected options. Packages that could have problems:

- Python 
- LibPCL
- opencv 
- libjpegturbo
- librealsense2
- Kinect for Azure and k4abt (body tracking)

Python requires a specific mention: if you have already installed a version of Python **and** that Python is on your **PATH** environment variable the cwipc Python interface modules will be installed into that Python installation. But again: if there is some incompatibility in the way your Python has been installed your only recourse is to uninstall it and let the cwipc installer re-install it.

> As is probably clear from this section, writing Windows installers is not our strong point. Please contact us if you can provide help.


### Linux

The installer is currently only available for Ubuntu 22.04.

Download the debian package for the most recent cwipc release from <https://github.com/cwi-dis/cwipc/releases/latest>.

Install from the command line with `sudo apt install ./yourpackagename.deb`.

The Kinect and Realsense SDKs will not be automatically installed, because they come from different repositories and not from the standard Ubuntu/Debian repositories. 

Inspect `/usr/share/cwipc/scripts/install-3rdparty-ubuntu2204.sh` to see how to install them.

### Mac

The installer is available via [Homebrew](https://brew.sh). Install with

```
brew tap cwi-dis/cwipc
brew install cwipc
```

Verify that everything (including the Python packages and scripts) is installed correctly by running

```
cwipc_view --version
```
 
It should be, but if there are issues with the Python packages you can manually (re-)install them by running 

```
cwipc_pymodules_install.sh
```

The github location of the brew recipe is at <https://github.com/cwi-dis/homebrew-cwipc>

### Android

The Android build of `cwipc` is API-only, and has only been tested with Unity applications running on Oculus Quest headsets. Pre-built releases are available via <https://github.com/cwi-dis/cwipc_android>.

## Using cwipc

After installation you have a set of command line utilities that you can use from the shell (or Windows command prompt) and a set of APIs that you can use in your C programs, C++ programs, Python programs or C#/Unity projects.

### Command line

Better documentation will be forthcoming. For now: run the program with `--help` argument. The main programs are:
- `cwipc_check` does a basic check of your cwipc installation, verifying everything has been installed correctly.
- `cwipc_calibrate` is used to setup your capturer for Realsense or Azure Kinect cameras.
- `cwipc_grab` is used to capture pointclouds from cameras, but also for converting, compressing, decompressing and a lot more.
- `cwipc_view` allows you to see your pointcloud stream. Either from camera, or played back from an earlier capture, or from a `cwipc_formward` stream and many other options.
- `cwipc_forward` streams pointclouds over the net.

### C or C++

Include files and libraries are installed in the standard places, and `pkgconfig` files are included. For example code: get a source distribution and look at `cwipc_util/apps`, `cwipc_codec/apps`, `cwipc_realsense2/apps`, etc.

### Python

The Python `cwipc` package should be installed in your default Python, otherwise you can do so by running `cwipc_pymodules_install.sh` (or `.bat`).

Python example code is installed in `share/cwipc/python/examples` where you will also find a readme file.

### Unity

At the moment the C# API is only packaged for use from Unity. Let us know if you have another application for it, then we can investigate `nuget` or something like that.

The cwipc Unity package lives in a separate repository, <https://github.com/cwi-dis/cwipc_unity>.

Install it by opening the Package Manager in the Unity Editor, _Add Package from git URL..._ and passing the URL ``git+https://github.com/cwi-dis/cwipc_unity?path=/nl.cwi.dis.cwipc``.

More complete instructions can be found at <https://github.com/cwi-dis/cwipc_unity/blob/master/nl.cwi.dis.cwipc/README.md> .



## Advanced installation: Installing a binary zip/tar distribution

If the installers do not fit your need you can install prebuilt binaries to a place of your liking.

Prebuilt binary releases are available at <https://github.com/cwi-dis/cwipc/releases> as zip or gzipped tar files. Download the correct one for you platform. On MacOS and Linux you can extract straight into `/usr/local` or any other location of your liking. On Windows you create an empty folder such as `C:/cwipc` and extract there.

- On Windows, add `c:/cwipc/bin` to your `%PATH%` environment variable (and restart your command prompt).

- On MacOS you need to clear the quarantine bits (which are Apple's way to ensure you cannot accidentally run malware downloaded from the internet):

  ```
  cd /usr/local # or wherever you extracted to
  xattr -d com.apple.quarantine bin/cwipc_*
  xattr -d com.apple.quarantine lib/libcwipc_*

  ```

- On Linux and Mac, if you did not install to `/usr/local`, add the `bin` directory to your `PATH` environment variable. You may also need to modify `LD_LIBRARY_PATH` or `DYLD_LIBRARY_PATH`.
- Optionally, if you want to use a python virtual environment so the cwipc modules and dependencies are not installed into your normal Python environment, create a Python venv:

  ```
  python3 -m venv venv
  . venv/bin/activate # Note the dot space. 
  ```
  
- Run `cwipc_pymodules_install.sh` to install the Python components. (On windows you can use this script when you are using bash, or you can run `cwipc_pymodules_install.bat` if you are using CMD).

- Check that everything is installed correctly by running

  ```
  cwipc_check
  cwipc_view --version
  cwipc_view --synthetic
  ```

  The last command should bring up a viewer window with a synthetic point cloud. Use left-mouse-drag, right-mouse-drag and scroll wheel to change your view position.


## Advanced usage: Installing third party requirements

Building from source requires `cmake`, `python3`, `libpcl`, `glfw3`, `jpeg-turbo` and optionally (for Intel Realsense support) `librealsense` and/or (for Azure Kinect support)  `Azure Kinect SDK`, `Azure Kinect Body Tracking SDK` and `OpenCV`.

Running binaries needs most of those requirements are well (but note that the installers should take care of all of these).

### Linux

There is a script `scripts/install-thirdparty-ubuntu2004.sh` that installs all requirements on Ubuntu 20.04. For other Linux variants please inspect this script and ensure the correct packages are installed.

### MacOS

There is a script `scripts/install-thirdparty-osx1015.sh` that installs all requirements on MacOS 10.15 or later. This script requires [HomeBrew](https://brew.sh) and the XCode Command Line Tools. Installing HomeBrew will help you install the command line tools.

Building and installing natively on Apple Silicon (M1 machines) might work and might not work. A workaround is to install HomeBrew for Intel (which can be installed alongside HomeBrew for M1, because they use different locations) and ensure that `/usr/local/bin` is in your `$PATH` before `/opt/homebrew/bin`. Then everything is built for Intel and Rosetta.

### Windows

There are two things you always need to install (independent of whether you want to use a binary installer or build from source):

- `git` and `bash`, from <https://git-scm.com/downloads>.
- Python, from <https://www.python.org/downloads>. 3.10 is preferred, as of this writing (April 2023) python 3.10 does not work because some required packages are not available for 3.11 yet.
	- Note: you should install Python *"For All Users"*. 
	- Note: You should install into a writeable directory, such as `C:/Python39` otherwise you will have to use _Run as Administrator_ for various build steps.

If you want to build from source you first need to install some developer resources:

- Visual Studio. Community Edition 2019 is known to work.
- CMake, from <https://cmake.org/install/>.

Next, you need to install the third-party libraries and tools mentioned above.

- Run the script `scripts/install-3rdparty-full-win1064.ps1` in a PowerShell **with Administrator rights**. Note the bold font.

Finally, you need to ensure that all DLLs from the packages installed above or on the environment `%PATH%` variable:

- Open the `install-3rdparty-full-win1064.ps1` script in a text editor, and inspect the comments that state what should be added to path.
- Open `Control Panel` -> `System Properties` -> `Environment Variables` -> `System Variables` -> `Path`.
- Check that each of the folders mentioned in the script exist (otherwise something went wrong during the installation step).
- Add each folder to the `Path`.
- Press `OK` to close the dialogs.
- Close all command prompt windows, bash windows and powershell windows and re-open them.

If you don't follow these steps you will later get obscure errors. Windows will tell you that (for example) `"The cwipc_realsense2 DLL could not be found"`, and you see it right in front of you. The _actual_ problem is going to be with one of the dependency DLLs (but it would be far to helpful if Windows told you this:-), and the problem invariably is that something has not been added to `Path`.

For the rest of the build instructions it is probably best to use `bash`, not `CMD` or powershell.


## Advanced usage: Building from source

You can either download a source archive (zip or gzipped tar) or clone the git repository.

### Download source archive

Full source releases (including submodules) are available at  <https://github.com/cwi-dis/cwipc/releases>, as assets with names like `cwipc-`_version_`-source-including-submodules`. Available as gzipped tar or zip, the contents are identical. Download and extract.

### Clone git repository

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

### Build using build script

You can run the usual `cmake`, `cmake --build`, `ctest`, `cmake --install` commands manually, or you can build by using one of the build scripts:

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
  
  As of this writing some of the dependencies are not available yet for the M1 chip, but building for Rosetta 2 works. Please _do_ ensure that Python for the correct architecture has been selected.

- Windows:

  ```
  bash ./scripts/buildall-win.sh
  ```
  This will build everything, do a limited self-test and install into `./installed`.
  
Both these scripts configures, builds, tests and installs each of the submodules individually (in `build` directories under the submodule directory). If you need to tweak the build procedure, for example by adding `cmake` flags, you can use `rm -rf build` to do a complete clean build.


## Debugging

Nowadays (2024) debugging with Python and VSCode may be the easiest way. See below.

A note here on how to debug the cwipc code, because it needs to go somewhere. When debugging it is easiest to build the whole package not with the command line tools but with Visual Studio (Windows) or Xcode (Mac). To debug with XCode create a toplevel folder `build-xcode` and in that folder run

```
cmake .. -G Xcode
open cwipc.xcodeproj
```

Some issues can then be debugged with the C or C++ command line utilities (by putting breakpoints at the right location and running them with the correct command line arguments).

Some issues are easier to debug with the Python scripts. There are some hooks in place to help with this:

- all Python scripts accept a `--pausefordebug` command line option. This will pause the script at begin of run (and end of run), waiting for you to press `Y`. While the script is paused you can obtain the process ID and attach the XCode or Visual Studio debugger to the process.
- all Python scripts accept a `--debuglibrary NAME=PATH` argument, for example `--debuglibrary cwipc_util=/tmp/libcwipc_util.dylib` to load the given cwipc library from the given path. This allows you to load the library that you have just built in Xcode or Visual Studio so you can set breakpoints in the library code.
- The python modules and scripts can be run from the `build` folder, using the venv-python there. As follows:

  ```
  cd build
  . venv/bin/activate
  python -m cwipc.scripts.cwipc_view --synthetic
  ```
  
  Such a run will pick up the dynamic libraries from the build folder, but you can also specify the debug options outlined above.
- The Python unittests can also be run individually from the build folder, for example with

  ```
  cd build
  . venv/bin/activate
  python ../cwipc_codec/python/test_cwipc_codec.py --verbose TestApi.test_cwipc_parallel_encoder
  ```

Additionally, you can send `SIGQUIT` to all the Python scripts (installed or when running from the build folder) to cause them to dump the Python stacktraces of all threads.

### Debugging with vscode

If you open the project with VSCode debugging the Python scripts is fairly easy. The main issue is that you need to ensure that the correct dynamic libraries are used (i.e. the ones that are built within this directory).

On Mac or Linux, in the VSCode terminal window (or the VSCode Python debugger terminal window), run

```
. scripts/activate.sh
```

On Windows powershell, use

```
&scripts\activate.ps1
```

Both of these will modify `PATH` or `DYLD_LIBRARY_PATH` or whatever to ensure the dynamic libraries built here take precedence over other versions. Also, they will activate the Python venv built here, and `pip install -e` the cwipc Python modules.

Debugging the Python code is now very easy: just run with the Python debugger from within VSCode.

Debugging the native code in a native app is also easy: again use the normal lldb debugger from within VSCode.

Debugging the native code when running within a Python app is slightly more convoluted:

- In the VSCode terminal window run the Python app with `--pausefordebug`. Take note of the PID.
- Run the `lldb` debugger in "Attach Process" mode, and specify that PID.
- Set any breakpoints you need.
- Type `Y` in the Python app to make it continue.

On Windows I have not been able to use the native debugger in this way, but using Visual Studio attack works, as explained in the previous subsection.

## Creating a release

These instructions are primarily for our own benefit. Lest we forget.

When creating a new release, ensure the following have been done

- Dependencies for the `.deb` installer for apt/Ubuntu need to be updated. There may be better ways to do this, but this works:
  - On the targeted Ubuntu, check out and edit `CMakeFiles/CwipcInstallers.cmake`
  - Comment out the definitions for `CPACK_DEBIAN_PACKAGE_DEPENDS` and `CPACK_DEBIAN_PACKAGE_RECOMMENDS`.
  - Un-comment-out `CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES`.
  - Build, cpack.
  - Extract the resulting debian package with `ar x`.
  - Unpack the `control.tar.gz` file.
  - Inspect the dependencies that cpack auto-generated.
  - Fix the dependencies and recommendations based on what cpack found.

- `scripts/install-3rdparty-full-win1064.ps1` should be updated to download the most recent compatible packages. Go through each of the packages, determine the current version. Uninstall old versions from your build machine. Run the powershell script to test it installs the new packages. Do the build, to ensure it works with the new packages. Test the build to ensure it runs with the new packages.

- `.github/workflows/build.yml` should be updated to download those same packages.

- `CWIPC_API_VERSION` incremented if there are any API changes (additions only).
- `CWIPC_API_VERSION_OLD` incremented if there are API changes that are not backward compatible.
	- Both these need to be changed in `api.h` and `cwipc/util.py`.
- `CHANGELOG.md` updated.

Version numbers for the release no longer need to be updated manually, they are generated from the git tag name.

After making all these changes push to github. Ensure the CI/CD build passes.

After that tag all submodules and the main module with *v_X_._Y_._Z_*.

Push the tag to github, this will build the release.

After the release is built copy the relevant new section of `CHANGELOG.md` to the release notes.

After that, update the `brew` formula at <https://github.com/cwi-dis/homebrew-cwipc>. Use `brew edit`, `brew install`, then push the changes. Things to change are the download URL, the sha, possibly the Python version (multiple places), possibly other dependencies.

Finally, when you are happy that everything works, edit the release on the github web interface and clear the `prerelease` flag.
