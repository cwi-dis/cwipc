# cwipc - CWI Point Clouds software suite

![build](https://github.com/cwi-dis/cwipc/actions/workflows/build.yml/badge.svg)

## Overview

In order to facilitate working with point clouds as opaque objects - similar to how most software works with images, or audio samples -  our group has developed an open source suite of libraries and tools that we call `cwipc` (abbreviation of CWI Point Clouds). The implementation builds on the PCL pointcloud library and various vendor-specific capturing libraries, but this is transparent to software using the `cwipc` suite (but it can access these representations if it needs to).

The idea behind a `cwipc` object is that it represents a point cloud (a collection of points with x/y/z coordinates, r/g/b values and possibly information on which camera angle the point was captured from, plus additional global information such as timestamp captured and voxel size, and optionally original RGB and D images or skeleton data). A cwipc object can be passed around without knowing what is inside it, and this can be done across implementation language boundaries while minimizing unnecessary memory copies. The library makes it possible to create end-to-end pipelines in order to capture, send, receive, and render dynamic point clouds. It is suitable for real-time applications and because point clouds can become very large special care is given to memory management and minimizing the amount of copying needed.

The core of the suite is `cwipc_util`, which handles the `cwipc` object implementation, its memory management and the multiple language bindings (C, C++, Python and C#). It also contains utility functions to read and write a `cwipc` object from a .ply file, apply different filters and transformations to the `cwipc` objects. In addition, it contains a set of tools (`cwipc_calibrate`) to align point clouds obtained from multiple cameras, a customized viewer (`cwipc_view`) to playback dynamic point clouds and a grabber tool (`cwipc_grab`) that allows you to grab point cloud frames from multiple devices or  from offline prerecorded files (which is what we used for creating the dataset).

The suite also contains modules `cwipc_kinect` and `cwipc_realsense2` that which capture point clouds from one or multiple cameras (Kinects and Realsense), and a module `cwipc_codec` that has the functionality to compress and decompress point clouds to make them suitable for real-time transmission.

## Unity

There is a separate repository <https://github.com/cwi-dis/cwipc_unity> that contains the Unity package needed to use cwipc from Unity. See there for instructions. You will still need to install this package.

If you ended up on this web page because you got an error from Unity that pointed you here: you are using some Unity project that uses `cwipc_unity` but the native package (this one) has not been installed correctly. See the _Installation_ section below.
## Use cases

The use cases for `cwipc` that we foresee and try to support:

- Creating C or C++ programs that capture point cloud streams, compress them, and transmit them over the net. On Linux, Windows, MacOS or Android.
- Create Python programs with the same functionality.
- Create Python programs or Jupyter notebooks to do analysis of point clouds or point cloud streams, for example using `numpy` or `pyopen3d`.
- Create Unity applications that can capture, compress, transmit, decompress and render live point cloud streams.
- Allow some of the above functionality to be used without any programming at all, through command line programs.

## More information

For now, refer to <https://www.dis.cwi.nl/cwipc-sxr-dataset/>.

Documentation on the API can be created using _Doxygen_ in `cwipc_util/doc`, and will be made available here at some point in the future.

The change log can be found at [CHANGELOG.md](CHANGELOG.md).

## Installation

The simplest way to install cwipc is through a prebuilt installer. This will install everything in the standard location, and it allows running the command line tools as well as developing C, C++, Python or Unity programs that use the cwipc library.

After installation, run `cwipc_view --synthetic` from a shell (terminal window, command prompt). It should show you a window with a rotating synthetic point cloud if everything is installed correctly. There is also a command line utility `cwipc_check` that will test that all third-party requirements have been installed correctly.
On Windows you can find these in the start menu too.

See below if you want to install to a different location, or if you want to modify cwipc itself and build it from source.

### Windows

Download the windows installer `.exe` for the most recent cwipc release from <https://github.com/cwi-dis/cwipc/releases/latest>.

Run it, and it will install the cwipc command line tools and the C++ and Python APIs.

> If the installer does not run you must install the "Microsoft VC++ Redistributable" first (64 bit version).

It will _also install all required third party packages_, unless a usable version is detected.

### Windows - check installation

Windows installers often fail because each Windows computer is different. Moreover, cwipc depends on a number of third party packages (such as the Realsense and Kinect support) that we cannot include in our installer because of licensing issues, so we have to rely on official installers for those packages.

After installing, run _Start menu_ -> _cwipc_ -> _Check cwipc installation_. This will open a CMD command window and try to find out if everything has been installed correctly. If there are any errors it may show a dialog which mentions which library has not been installed correctly. And there may be error messages in the output window.

If this shows any errors, try _Attempt to fix cwipc installation_. 

> As of July 2024 there is a problem you should check for first, which is not fixed by the automatic fixer.
> 
> You should go to _Apps_ -> _Installed apps_ and check that your version of _Microsoft Visual C++ 2015-2022 Redistributable (x64)_ is at least version 14.40.33810.0. If your installed version is older: update. Searching for _MSVC redist_ will find the download links.
> 
> This needs to be done because unfortunately Microsoft has made an incompatible change to their C++ Runtime, so any program built after about May 2024 will crash if it uses an older version of the runtime.

If after that the check command still fails, the problem is probably that one of the third party packages _is_ installed on your computer, but it is an incorrect version, or it is installed in a different way than what _cwipc_ expects.

Try to determine which package is responsible for the failure, and uninstall it. Then reboot and re-try the _fix cwipc installation_. This _should_ install the correct version of every package, and install it with the expected options. Packages that could have problems:

- Python 
- LibPCL
- librealsense2
- Kinect for Azure and k4abt (body tracking)

Python requires a specific mention: if you have already installed a version of Python **and** that Python is on your **PATH** environment variable the cwipc Python interface modules will be installed into that Python installation. But again: if there is some incompatibility in the way your Python has been installed your only recourse is to uninstall it and let the cwipc installer re-install it.

And actually Realsense also requires a specific mention: if you already have it installed but have have a different version than what cwipc expects your only recourse is to uninstall it and then re-run the cwipc installer so it will install the correct version.

> As is probably clear from this section, writing Windows installers is not our strong point. Please contact us if you can provide help.


### Linux

The installer is currently available for Ubuntu 24.04 and 22.04.

> Note that some packages are missing for 24.04. Specifically, the Kinect capturer cannot be used because the needed SDK from Microsoft is not available, and probably will never be made available because the Kinect is no longer supported by Microsoft.
> 
> Also, the Realsense SDK is not yet available for 24.04. So essentially you will have no access to any cameras when using 24.04.

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

### Setting up your cameras

Initial documentation on setting up your cameras can be found in [Setting up your cameras](doc/registration.md).

### Command line

Better documentation will be forthcoming. For now: run the program with `--help` argument. The main programs are:

- `cwipc_check` does a basic check of your cwipc installation, verifying everything has been installed correctly.
- `cwipc_register` is used to setup your capturer for Realsense or Azure Kinect cameras.
- `cwipc_grab` is used to capture pointclouds from cameras, but also for converting, compressing, decompressing and a lot more.
- `cwipc_view` allows you to see your pointcloud stream. Either from camera, or played back from an earlier capture, or from a `cwipc_forward` stream and many other options.
- `cwipc_forward` streams pointclouds over the net.

#### Recording

> This will need to go somewhere better. 

You can record your camera capture streams (both Realsense and Kinect) while you are capturing the streams in any program using _cwipc_, so not only `cwipc_view` but also anything like a Unity app using the cameras. Create a directory next to your `cameraconfig.json` file, let's say `recording`, and then set the cameraconfig field `record_to_directory` to `"recording"` (could also be an absolute path).

Now run your application as usual. 

Clear the `record_to_directory` field again. Copy `cameraconfig.json` into the `recording` directory. Change camera and capturer `type` to `kinect_offline` or `realsense_playback`. Add the `filename` field to each `camera` entry. For a single Kinect you may have to set `ignore_sync` to `true`.

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
  . venv/bin/activate # Note the space after the dot...
  ```
  
- Run `cwipc_pymodules_install.sh` to install the Python components. (On windows you can use this script when you are using bash, or you can run `cwipc_pymodules_install.bat` if you are using CMD). Also, for Windows, it may be best to run this as `Administrator`.

- Check that everything is installed correctly by running

  ```
  cwipc_check
  cwipc_view --version
  cwipc_view --synthetic
  ```

  The last command should bring up a viewer window with a synthetic point cloud. Use left-mouse-drag, right-mouse-drag and scroll wheel to change your view position.


## Advanced usage: Installing third party requirements

Building from source requires `cmake`, `python3`, `libpcl`, `glfw3`, `jpeg-turbo` and optionally (for Intel Realsense support) `librealsense` and/or (for Azure Kinect support)  `Azure Kinect SDK`, `Azure Kinect Body Tracking SDK` and `OpenCV`.

Running binaries need most of those requirements are well, but the installers should take care of all of these.

### Linux

There is a script `scripts/install-thirdparty-ubuntu2004.sh` that installs all requirements on Ubuntu 20.04. For other Linux variants please inspect this script and ensure the correct packages are installed.

### MacOS

There is a script `scripts/install-thirdparty-osx1015.sh` that installs all requirements on MacOS 10.15 or later. This script requires [HomeBrew](https://brew.sh) and the XCode Command Line Tools. Installing HomeBrew will help you install the command line tools.

Building and installing should work for both Apple Silicon (M1 machines) and Intel machines.

### Windows

Building on Windows is best done with _Visual Studio Code_ because it will help you install most of the other requirements for development (cmake, git, etc).

Install the following:

- Python, from <https://www.python.org/downloads>. 3.11 is preferred, as of this writing (March 2024).
	- Note: you should install Python *"For All Users"*. 
	- Note: You should install into a writeable directory, such as `C:/Python39` otherwise you will have to use _Run as Administrator_ for various build steps.
- Visual Studio Code, from <https://code.visualstudio.com>
- In VSCode, install:
	- C/C++ Tools and Extension Pack
	- CMake and CMake Tools
	- Python, Pylance and Python Debugger
	
#### Windows without vscode

There are a few things you need to install before building from source on Windows:

- Visual Studio. Community Edition 2022 is known to work, but probably anything after 2019 will work.
- CMake, from <https://cmake.org/install/>.
- Python, from <https://www.python.org/downloads>. 3.11 is preferred, as of this writing (March 2024).
	- Note: you should install Python *"For All Users"*. 
	- Note: You should install into a writeable directory, such as `C:/Python39` otherwise you will have to use _Run as Administrator_ for various build steps.
- `git` and `bash`, from <https://git-scm.com/downloads>. We are not quite sure whether these are actually required...
- Visual Studio Code is not really needed, but debugging or developing the Python code is much easier with VSCode than with VS.

Next, you need to install the third-party libraries and tools mentioned above.

- Run the script `scripts/install-3rdparty-full-win1064.ps1` in a PowerShell **with Administrator rights**. Note the bold font.

For the rest of the build instructions it is probably best to use `bash`, not `CMD` or powershell.


## Advanced usage: Building from source

You can either download a source archive (zip or gzipped tar) or clone the git repository. Note that the latter is preferred.

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

### Build using vscode

When you have the project open in vscode select the correct CMake preset you want to build (_Command Palette..._ -> _CMake: Select configure preset_).

Then _CMake: Build_.

When building for Windows or Android this will first install all the required dependencies using `vcpkg`. This can take quite some time the first time you build on a machine (think: an hour or so) but the vcpkg builds are cached so the next time it will go a lot quicker.


### Build using build script

You can use the usual `cmake`, `cmake --build`, `ctest`, `cmake --install` commands. There are _cmake presets_ for the various platforms and use cases (development or release). Use `cmake --list-presets` to see the ones which are valid for your platform.

On Linux and Macos this will install into `/usr/local` on Windows it will install into `../installed` by default.

You can also build right from Visual Studio Code using the cmake plugin.


## Debugging

Nowadays (2024) debugging with Python and VSCode may be the easiest way. See below.

### Debugging with Visual Studio or XCode

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

Debugging the Python code is now very easy: just run with the Python debugger from within VSCode. You may have to tell vscode about the interpreter to use, from your `./build/venv` environment.

Debugging the native code in a native app is also easy: again use the normal lldb or Windows debugger from within VSCode.

Debugging the native code when running within a Python app is slightly more convoluted:

- In the VSCode terminal window run the Python app with `--pausefordebug`. Take note of the PID.
- Run the `lldb` debugger in "Attach Process" mode, and specify that PID.
- Set any breakpoints you need.
- Type `Y` in the Python app to make it continue.

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
- For Windows, the `vcpkg` dependent packages should all be updated to the most recent version.

  ```
  cd .\vcpkg
  git pull
  .\bootstrap-vcpkg.bat
  cd ..
  .\vcpkg\vcpkg.exe install
  git commit -a -m "Vcpkg packages updated to most recent version"
  ```

- `setup.py` may still have a version string somewhere.

- `CWIPC_API_VERSION` incremented if there are any API changes (additions only).
- `CWIPC_API_VERSION_OLD` incremented if there are API changes that are not backward compatible.
	- Both these need to be changed in `api.h` and `cwipc/util.py`.
- `CHANGELOG.md` updated.

Version numbers for the release no longer need to be updated manually, they are generated from the git tag name.

After making all these changes push to github. Ensure the CI/CD build passes. This build will take a looooong time, most likely, because the `vcpkg` dependencies have been updated and the Windows runner will have to rebuild the world.

Now do a nightly build, using `scripts/nightly.sh`.

After that tag all submodules and the main module with *v_X_._Y_._Z_*.

Push the tag to github, this will build the release.

After the release is built copy the relevant new section of `CHANGELOG.md` to the release notes.

After that, update the `brew` formula at <https://github.com/cwi-dis/homebrew-cwipc>. Use

- `brew edit cwipc` and change the URL and version (and possibly Python or other dependencies),
- `brew fetch cwipc` to get the error about the SHA mismatch, fix the SHA,
-  `brew install` to install the new version,
-  then push the changes (easy from within `vscode`),
-  then `brew upgrade cwipc` on another machine to test.

Finally, when you are happy that everything works, edit the release on the github web interface and clear the `prerelease` flag.
