# Building from Source

This section describes how to compile **cwipc** from the GitHub repository on
macOS, Linux or Windows.  The project uses CMake with a vcpkg‑based third
dependency tree.

## Prerequisites

* A recent CMake (3.20+).
* A C++17 compiler (clang, gcc, MSVC).
* Git toolchain.
* `bash` or `zsh` on macOS/Linux; PowerShell on Windows.

## Steps

1. Clone the repository:

   ```sh
   git clone https://github.com/cwi-dis/cwipc.git
   cd cwipc
   ```

2. Install third‑party packages:
   * macOS: `scripts/install-3rdparty-macos.sh`
   * Ubuntu: `scripts/install-3rdparty-ubuntu2404.sh`
   * Windows: `scripts/install-3rdparty-full-win1064.ps1` (run as Administrator).

3. Create a build directory and configure with CMake presets:

   ```sh
   mkdir build && cd build
   cmake --preset release        # or `debug`, `msvc-release` on Windows
   ```

   The presets automatically supply the vcpkg toolchain.

4. Build the code:

   ```sh
   cmake --build --preset release
   ```

   On Windows this will produce a Visual Studio solution under `build/`.

5. (Optional) Run the unit tests:

   ```sh
   cmake --build --preset release --target RUN_TESTS
   ```

6. Install to a local prefix:

   ```sh
   cmake --build --preset release --target install
   ```

   The default prefix is `/usr/local` on Unix and `C:\Program Files\cwipc` on
   Windows; override with `-DCMAKE_INSTALL_PREFIX` when invoking `cmake`.

## Notes

* Use the `CMAKE_OPTIONS` field in `CMakeUserPresets.json` to toggle features
  (for example `-DCWIPC_WITH_GUI=OFF`).
* Python bindings are built as part of the main build and packaged by the
  `cwipc_pymodules_install.sh` script.
* To build Android targets, use the corresponding preset `android-release`.

[Back to Contents](index.md)
