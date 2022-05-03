# Changelog
All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

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

