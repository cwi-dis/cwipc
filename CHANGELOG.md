# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [unreleased]

- Use different vcpkg caching strategy for Github Actions (#197)
- Windows "built" has been added to the release assets again. (#187)
- Realsense depth erosion fixed. (#201)
- Somewhat fixed cwipc_register --guided instructions. (#194, #199)
- Implemented --inpoint for realsense playback (#174) but it has issues with cwipc_grab.
- Various fixes to help with lldash integration (#198, more)

## [7.6.5] - 2025-03-20

- cwipc_view was broken. Fixed.

## [7.6.4] - 2025-03-18

- Many improvements to registration (#18)
- New command `cwipc_register --guided` which should handle most use cases for camera registration.

## [7.6.3] - 2025-02-21

### Changed

- Changed dates in Changelog, they were off by 10 years:-)
- Android build fixed (#179, #172)

## [7.6.0] - 2025-01-19

### Added

- Realsense capturer can now record during live capture, using `system.record_to_directory` config setting. (#119)
- Debian packages for Ubuntu 24.04 (#143), for the time being without Kinect and Realsense support.
- Android-arm64 build added (#91). Only liibraries, really only for use with cwipc_unity.
- New tool `cwipc_timing` helps with determining whether cameras are correctly synchronized. (#144)

### Changed

- We will try to create micro-releases much more often from now on, so people do not have to use nightly builds
- Minor documentation fixes
- More resilience against incomplete installations (#108)
- Python 3.12 and NumPy 2 support (#101, #109, #114)
- Fixed colors for `cwipc_view --rgb` with RealSense camera (#115)
- Fixed `cwipc_register` for multiple RealSense cameras (#106)
- Fixed `cwipc_synthetic` reader to not wait on available(False) (#117)
- Realsense `cameraconfig.json` file rationalized. Major code cleanup. (#137, #125, #118)
- Calibration of Realsense D455 cameras fixed. (#127)
- Use cmake presets for building (#120)
- Use `pyproject.toml` for Python package generation (#79)
- Implemented recording while capturing for both Realsense and Kinect (#139)
- Glfw is now used via normal dependencies, no longer slurped (#123)
- `cwipc_forward` can now serve streams to multiple connections at the same time (#151)
- Major improvements to realsense_playback (#158)

## [7.5.3] - 2024-04-30

### Changed

- Fixed Mac brew builds (#105)
- Updated vcpkg dependencies

## [7.5.2] - 2024-04-29

### Added

- Registration procedure documented (#103)

### Changed

- Do not attempt fine calibration with a single camera (#104)

## [7.5.1] - 2024-03-27

### Added

- New Python methods (on cwipc objects) `as_numpy_array()`, `as_numpy_matrix()`, `as_o3d_pointcloud()` and corresponding constructors `from_numpy_array()`, `from_numpy_matrix()` and `from_o3d_pointcloud()` to allow easier analysis with numpy or open3d.
- New Python methods on `cwipc_auxiliary_data` objects `get_image()` and `get_all_images()` allow you to get the RGB and Depth images as numpy arrays.
- New API call `cwipc_tiledsource_auxiliary_operation` to have a capturer do a special operation, such as mapping 2D points to 3D points.
- There is a new utility `cwipc_register` to do registration of multiple cameras (replacing the old `cwipc_calibrate`).
- There is a new capturer `realsense_playback` which reads `.bag` recordings captured earlier. It behaves much more like a camera than the older `realsense_offline`.
- Cameraconfig files are now JSON by default (but XML is still supported for backward compatibility).
- The default camera type is now determined from `cameraconfig.json`, so you normally don't have to specify `--kinect` or similar to `cwipc_view` and other utilities
- There is a new API call `cwipc_capturer` that will return the correct capturer for what is specified in `cameraconfig.json`. You can also give it a `configFilename` parameter of `"auto"` and it will determine what type of camera is attached to your system.
- cwipc has been ported to Android, specifically for use on the Oculus Quest. 
- Python modules now have type annotations, which should make it much easier to use cwipc in your programs (auto-complete, documentation, etc). The cwipc package is usable from Jupyter.
- Python example programs have been added.
- `cwipc_check` program helps with checking that everything is installed (especially on Windows). Windows Start Menu has entries to do this check, and also to try and fix issues.

### Changed

- Build process on Windows now uses vcpkg for most dependencies, streamlining both CI/CD builds and local builds.
- Build process and CMakefiles have been streamlined, specifically the way the Python support is installed.
- command line arguments to `cwipc_view` and other utilities have changed a lot. Use `--help` to see the differences.
- Windows installer should now be more robust.

### Removed

- `cwipc_calibrate` is deprecated, replaced by `cwipc_register`.
- `cwipc_unity` is no longer a submodule of cwipc but its own toplevel repository.

## [7.4.2] - 2023-05-07

### Added

- cwipc\_unity submodule added. Can be added to Unity projects either via inclusion by path from a checkout or by adding the cwipc_unity repo URL.
- Optional parallel encoding of subsequent point clouds. Does not decrease latency but can increase throughput.

### Changed

- Ubuntu 22.04 is now the Linux release for which we provide packages.
- Python 3.10 is now preferred, 3.9 still supported.
- Dependencies updated to current versions.
- Various issues with Windows installer fixed.
- Tweaks to allow easier building with brew on macos.

### Removed

Gitlab CI/CD no longer supported.

## [7.3.7] - 2022-07-14

### Added

- Installers for Windows (NSIS exe) and Ubuntu (debian package) are available in the release assets.
- Installer for Mac is available through Homebrew, see the readme file.
- The encoder can be run in parallel on mutliple threads, by setting `cwipc_encoder_params.n_parallel` to a value greater than `1`.

### Changed

- `CWIPC_ENCODER_PARAM_VERSION` has been increased, but the old version number is still accepted.

## [7.2] - 2022-05-03

### Added

- All tools accept a `--version` argument to only print the version. `cwipc_get_version()` added to API, `CWIPC_API_VERSION` updated. 
- Github is now the primary hosting platform and Gitlab is considered a mirror. Binary and full-source releases are available on github via <https://github.com/cwi-dis/cwipc/releases>.
- Instructions for installing from binary or source releases updated.

### Changed

- (kinect) cameraconfig is now independent of whether we use map\_color\_to\_depth or not
- (kinect) implemented function `generate_point_cloud_v2()` which uses a lookup table to compute the pointcloud.
- (kinect) opencv is now used for filtering the depth map: Thresholds + erosion. much faster.
- (macos) MacOS 10.15 is now the target platform: 10.14 is no longer easily supported in brew.
- Build process, CMakefiles and versioning have been streamlined.

## [7.1] - 2022-01-26

Backwards compatible, so only `CWI_API_VERSION` has been increased.
### Added

- cwipc_crop added


## [7.0] - 2022-01-05

This release is not backwards compatible with older releases, therefore `CWIPC_API_VERSION` and `CWIPC_API_VERSION_OLD` have been updated.

### Added

- Compatible with Python 3.9 (mainly DLL search path fixes for Windows)
- Added `--custom_filter` to most tools to allow applying a filter to each pointcloud
- Added skeleton data renderer to `cwipc_view`
- `cwipc_enc_perftest` (source only) helps debugging codec performance issues
- Kinect skeleton parameters can be specified in `cameraconfig.xml`

### Changed

- natural pointcloud orientation has changed: positive z-axis now points forward from subject point of view. This is in line with standard practice (camera looks in negative z direction). Incompatible change.
- `tileinfo` structure changed. Incompatible change.
- K4ABT 1.1.0 supported
- Got rid of `abort()` calls in capturers
- Code resutructuring in `cwipc_codec`
- Various other bug fixes

### Removed

## [6.4] - 2021-10-29

### Added

- Added compressed pointcloud support to `cwipc_grab` and `--playback`.
- Added methods `cwipc_colormap`, `cwipc_tilemap` and `cwipc_join`.

### Changed

- Minor bug fixes.

## [6.3.2] - 2021-10-14

### Changed

- Compressed pointclouds could lose their pointsize, causing them to be rendered with tiny size. Fixed.
- Changes for debugging cwipc itself, see readme.md for details.

## [6.3.1] - 2021-09-26

### Changed

- Compatible with PCL 1.12

## [6.3] - 2021-08-17

### Added

- `.pdb` files included on Windows, so stack traces and debugging are better.
- implemented disabled config variable for most capturers.

### Changed

- `cwipc_encodergroup` optimised and parallelised: can now handle 16-stream tiled multi-quality encoded pointcloud streams at 15fps (given decent hardware).

### Removed

## [6.2] - 2021-08-05

### Added

- Parallelized encodergroup if more than 1 output required.

### Changed

- fixed rebuild issue.
- Realsense and offline grabbers now also support disabled parameter in cameraconfig.

## [6.1] - 2021-07-21

### Added

- Allow disabling kinect camers with `enabled="0"` in `cameraconfig.json`.
- Added outlier filter.

### Changed

- Fixed skeleton API.

## [6.0] - 2021-06-21

First unified release of *cwipc\_util*, *cwipc\_realsense2*, *cwipc\_codec* and *cwipc\_kinect*.

Dual-homed at <https://gitlab.com/VRTogether_EU/cwipc/cwipc> and <https://github.com/cwi-dis/cwipc>.

MIT license added.

### Added

- New utility `cwipc_forward` to forward pointcloud streams over net (direct sockets or DASH using bin2dash)
- `cwipc_view` and friend can receive pointcloud streams over the net (direct sockets or DASH using *signals-unity-bridge*)

### Changed

- Fixes and additions to utilities, including support for Kinect skeleton and RGBD data.
- Unified build procedure using `cmake`.

