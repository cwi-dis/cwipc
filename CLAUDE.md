# cwipc

Low-level dynamic point cloud infrastructure: a cross-platform C/C++ suite, with Python and
C# bindings, for capturing, compressing, transmitting, decompressing and rendering live point
clouds. Developed by the CWI DIS group. This file covers `cwipc` itself.

## Ecosystem context

`cwipc` is also the base layer under `cwipc_unity` (a Unity bridge) and the VR2Gather social
XR framework, both developed by the same group. If you have access to the private
`VR2Gather_umbrella` repo (CWI-DIS-internal) checked out as a sibling directory, its
`CLAUDE.md` (visible here as `../CLAUDE.md`) has the full picture of how this repo relates to
those — use it when available.

## What it is

A cross-platform (Windows/macOS/Linux/Android) C/C++ suite, with Python and C# bindings,
for treating point clouds as opaque objects — capture, compress, transmit, decompress,
render — without callers needing to know the internal representation. Built on PCL plus
vendor capture SDKs, but that's hidden behind the API.

Full narrative overview: `docs/introduction.rst`. Full API reference: `docs/api-overview.rst`,
`docs/api-python.rst`, `docs/api-csharp.rst`.

## Repository layout

All modules live as plain subdirectories (this used to be five separate repos joined by
git submodules — flattened in #332 / release 8.1.0, 2026-08). The **only** remaining
submodule is `vcpkg`.

| Directory | Purpose |
|---|---|
| `cwipc_util` | Core: the `cwipc_pointcloud`/`cwipc_source`/`cwipc_sink` object model, memory management, C/C++/Python/C# bindings, `.ply` I/O, filters, the `cwipc` CLI (register/view/grab/copy/forward/...). Also home to a fair amount of genuinely non-core utility code that just found it convenient to live here — don't assume everything under `cwipc_util` is part of the core API |
| `cwipc_codec` | Compression/decompression, based on a modified `cwi-pcl-codec` (Mekuria et al., IEEE TCSVT 2017) |
| `cwipc_kinect` | Azure Kinect capturer. **Sunsetting**: hardware is discontinued; builds on Windows 11 only as of 2026 |
| `cwipc_orbbec` | Orbbec Femto capturer — the newest/active capturer. Needs OrbbecSDK v2 |
| `cwipc_realsense2` | Intel RealSense capturer. Needs the RealsenseAI fork of librealsense |
| `vcpkg` | C++ dependency manager, vendored as a submodule |
| `sandbox/recording-{kinect,orbbec,realsense2}/` | Real camera recordings (git-LFS) + `cameraconfig.json`, for dev/testing without hardware attached |
| `docs/` | Sphinx source, published at readthedocs.io; build locally with `docs/makedoc.sh` |
| `scripts/` | 3rd-party install scripts per platform, `nightly.sh` (retags+pushes the `nightly` release tag to trigger CI) |

Each `cwipc_*` module follows the same internal shape: `include/` (public headers),
`src/`, `apps/` (CLI subcommand implementations), `python/`, `tests/`.

## Reorg in progress — don't assume this structure is final

This repo is partway through a multi-step restructuring (umbrella issue #342):

1. ✅ Flatten submodules into subdirectories (#332, done)
2. ✅ Onboard Claude to the repo (#343, done — this file is part of that)
3. ⬜ Route all cross-language API calls through `cwipc_util` (old direct-to-capturer/codec
   calls stay temporarily for back-compat, removed in a later `v8.x`)
4. ⬜ Reorganize into: core (current `cwipc_util`) / capturers+codecs / language bindings,
   each with their own native+Python tests
5. ⬜ Rationalize the `cwipc_pointcloud`/`cwipc_source` API surface for cross-language ergonomics
6. ⬜ Integrate @pbauszat's in-progress work (currently developed elsewhere as a parallel stopgap)

Steps 3–6 don't have sub-issues yet (to be created later). If a task touches API structure
or module boundaries, check whether it should wait for or fold into one of these steps rather
than adding to the pre-reorg structure.

## Core API concepts

Everything centers on a few opaque C++ classes exposed through a C API
(`cwipc_util/include/cwipc_util/api.h`):

- `cwipc_pointcloud` — the point cloud object itself (points + metadata: timestamp, tile info,
  optional RGB/D images or skeleton data). Reference-counted; Python wrapper handles this
  automatically (no manual `free()` since 8.0).
- `cwipc_source` / `cwipc_activesource` — anything that produces point clouds (camera,
  network client, file playback, synthetic generator). `cwipc_activesource` requires `start()`
  before reading.
- `cwipc_sink` — anything that consumes point clouds (encoder, network sender, file writer).
- `cwipc_metadata` — the "metadata"/"auxdata" attached to a point cloud.

Naming note: `cwipc_pointcloud` (not `cwipc`) and `cwipc_activesource` (not `cwipc_tiledsource`)
are the post-8.0 names; older code/docs may use the old ones.

## Building

Full instructions (prerequisites, per-platform 3rd-party install scripts, LFS/vcpkg setup):
`docs/build-from-source.rst`. In short: CMake + presets (`cmake --list-presets`, then e.g.
`cmake --preset mac-production`), vcpkg toolchain auto-supplied by the preset. Selective
builds are controlled by `CWIPC_WITH_{APPS,PYTHON,GUI,TESTS,CAPTURERS,KINECT,REALSENSE,ORBBEC}`
CMake options (see top of `CMakeLists.txt`), or `CWIPC_API_ONLY` to build just the libraries.

Tests are wired through CTest (`add_test`, gated by `CWIPC_WITH_TESTS`); Python tests
additionally run via `unittest` — see `cwipc_util/python/test_cwipc_util.py` as the pattern
(fixtures under `<module>/tests/fixtures`).

## CLI tools

One `cwipc` entry point with git-style subcommands (`check`, `register`, `grab`, `view`, `play`,
`copy`, `forward`, `netserver`, ...). Full reference with examples: `docs/cli-tools.rst`
(streaming, conversion/filtering) and `docs/raw-recording.rst` (recording/playback) and
`docs/registration.rst` (multi-camera calibration).

## Python bindings

Live at `cwipc_util/python/cwipc/` (`util.py`, `codec.py`, `kinect.py`, `orbbec.py`,
`realsense2.py`, `playback.py`, `abstract.py`). Runnable examples in
`cwipc_util/python/examples/`.

## State / active work

Currently on release **8.1.0** (2026-08-24). See `CHANGELOG.md` for release history and
`gh issue list --repo cwi-dis/cwipc` for open work — notably #341 (release automation broken),
#331/#325 (Windows CI Python test failures, currently disabled), and #334 (pylint gate not
yet wired into testing).
