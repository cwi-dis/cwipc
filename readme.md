# cwipc - CWI Point Clouds software suite

![build](https://github.com/cwi-dis/cwipc/actions/workflows/build.yml/badge.svg)

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

If after that the check command still fails, the problem is probably that one of the third party packages *is* installed on your computer, but it is an incorrect version, or it is installed in a different way than what *cwipc* expects. This will most likely be Python.

Try to determine which package is responsible for the failure, and uninstall it. Then reboot and re-try the *fix cwipc installation*. This *should* install the correct version of every package, and install it with the expected options. Packages that could have problems:

- Python 
- Kinect for Azure and k4abt (body tracking)

Python requires a specific mention: if you have already installed a version of Python **and** that Python is on your **PATH** environment variable the cwipc Python interface modules will be installed into that Python installation. But again: if there is some incompatibility in the way your Python has been installed your only recourse is to uninstall it and let the cwipc installer re-install it.

And actually Realsense also requires a specific mention: if you already have it installed but have have a different version than what cwipc expects you will get an error message from *check installation* but you should be able to ignore it.

> As is probably clear from this section, writing Windows installers is not our strong point. Please contact us if you can provide help.

### Debugging Windows installation issues

If all the tips above do not help you should try installing `ProcessMonitor` from `SysInternals` (Google will help you find it).
This tool is not for the faint of heart, but it allows you to trace each and every system call on your computer. So it *will* allow you to trace why weird things like "the cwipc utilities work but I cannot use the realsense capturer from within Unity" happen.


## Creating a release

These instructions are primarily for our own benefit. Lest we forget.

When creating a new release, ensure the following have been done

- Python dependencies and Python maximum version need to be updated:
	- in `cwipc_util/python/pyproject.toml` remove the dependency specifiers,
	- build on all platforms, ensure everything works, possibly lowering the versions of some dependencies, and possibly lowering the maximum Python version
		- Especially watch out for `opencv` and `open3d` which can some times lag 2 Python versions
	- Add the Python package dependency specifiers again for the currently selected versions.
	- Update the Python version range in the toplevel `CMakeLists.txt`.
	- Update `scripts/install-3rdparty-windows.ps1` with the best Python version.
	- Update `scripts/install-3rdparty-macos.sh` with the best Python version.
	- Check the Ubuntu install-3rdparty scripts for which Python they install.
	- Check `.github/workflows/build.yml` for the Python versions used.
- Check whether `nlohman_json` can be updated (`CMakeLists.txt`)
- Check whether `nsis` can be updated (`.github/workflows/build.yml`)
- Dependencies for the `.deb` installer for apt/Ubuntu need to be updated. There may be better ways to do this, but this works:
  - On the targeted Ubuntu, check out and edit `CMakeFiles/CwipcInstallers.cmake`
  - Comment out the definitions for `CPACK_DEBIAN_PACKAGE_DEPENDS` and `CPACK_DEBIAN_PACKAGE_RECOMMENDS`.
  - Un-comment-out `CPACK_DEBIAN_PACKAGE_SHLIBDEPS YES`.
  - Build, run cpack with
    ```
    cpack --config build/CPackConfig.cmake -D CPACK_DEBIAN_FILE_NAME="cwipc_test_ubuntu2404.deb"
    ```
  - Extract the resulting debian package with `ar x`.
  - Unpack the `control.tar.gz` file.
  - Inspect the dependencies that cpack auto-generated.
  - Fix the dependencies and recommendations based on what cpack found.

- `scripts/install-3rdparty-windows.ps1` should be updated to download the most recent compatible packages. Go through each of the packages, determine the current version. Uninstall old versions from your build machine. Run the powershell script to test it installs the new packages. Do the build, to ensure it works with the new packages. Test the build to ensure it runs with the new packages.
  > Note: the only package that is important here nowadays is Python, because the other other two left here, `k4a` and `k4abt`, are no longer maintained.
- For Windows and Android, the `vcpkg` dependent packages should all be updated to the most recent version.

  ```
  cd .\vcpkg
  git pull
  .\bootstrap-vcpkg.bat
  cd ..
  .\vcpkg\vcpkg x-update-baseline
  .\vcpkg\vcpkg.exe install
  git commit -a -m "Vcpkg packages updated to most recent version"
  ```

- The toplevel `vcpkg.json` has a version string.
- `setup.py` in `cwipc_util/python` and every other package has a default version string that is only used when installing with `pip install -e` (because usually it is dynamically determined at build time. For good measure update these default version strings when doing a major release.
- `CWIPC_API_VERSION` incremented if there are any API changes (additions only).
- `CWIPC_API_VERSION_OLD` incremented if there are API changes that are not backward compatible.
	- Both these need to be changed in `api.h` and `cwipc/util.py`.
- `CHANGELOG.md` updated.

Version numbers for the release no longer need to be updated manually, but note the exceptions above.

After making all these changes push to github. Ensure the CI/CD build passes (easiest is by running `./scripts/nightly.sh` which does a nightly build). This build will take a looooong time, most likely, because the `vcpkg` dependencies have been updated and the Windows runner will have to rebuild the world.

After that tag all submodules and the main module with *v_X_._Y_._Z_*.

If one of the next steps fails just fix the issue and do another micro-release. Has happened to me every single release, I think:-)

Push the tag to github, this will build the release.

After the release is built copy the relevant new section of `CHANGELOG.md` to the release notes.

After that, update the `brew` formula at <https://github.com/cwi-dis/homebrew-cwipc>. Use

- `brew edit cwipc` and change the URL and version (and possibly Python or other dependencies),
- `brew fetch cwipc` to get the error about the SHA mismatch, fix the SHA,
-  `brew install` to install the new version,
-  then push the changes (easy from within `vscode`),
-  then `brew upgrade cwipc` on another machine to test.

Finally, when you are happy that everything works, edit the release on the github web interface and clear the `prerelease` flag.
