# Copilot / AI agent instructions for cwipc

Purpose: Quickly orient an AI coding agent to the cwipc repository so it can make focused, correct edits.

Big picture
- **Core modules:** `cwipc_util` is the core runtime and API. `cwipc_codec` handles compression; `cwipc_kinect` and `cwipc_realsense2` are capturers. Each module has `apps/` with example CLI tools.
- **Build & packaging:** CMake-based multi-target project. Uses `vcpkg` (repo-local `vcpkg/`) for dependencies and produces a Visual Studio solution under `build/` on Windows.
- **Runtime:** Command-line tools such as `cwipc_view`, `cwipc_forward`, `cwipc_grab`, and `cwipc_check` are the main runtime entrypoints. Unity integration lives in a separate repo `cwi-dis/cwipc_unity`.
- **Main CLI:** the top-level `cwipc` Python script is the primary command-line entrypoint. It dispatches subcommands (for example `view`, `grab`, `forward`, `check`) and is typically installed on the PATH. Run `cwipc --help` to list subcommands. Examples: `cwipc view --synthetic`, `cwipc forward --port 4303`, `cwipc check`.

Where to look first (concrete files)
- Top-level overview and build notes: [readme.md](readme.md)
- Windows third-party installer scripts: [scripts/install-3rdparty-full-win1064.ps1](scripts/install-3rdparty-full-win1064.ps1)
- vcpkg integration and ports: `vcpkg/` and [vcpkg.json](vcpkg.json)
- Example module CMake: [cwipc_util/src/CMakeLists.txt](cwipc_util/src/CMakeLists.txt)
- Examples and apps: `cwipc_*/apps/` directories
- Build outputs and VS solution: `build/` (contains `cwipc.sln` and `RUN_TESTS.vcxproj`)

Developer workflows (procedural, reproducible)

Prefer using CMake presets for normal development workflows: the repository includes `CMakePresets.json` (and you may create a local `CMakeUserPresets.json`). Use `cmake --preset <name>` to configure and `cmake --build --preset <name>` to build with the same preset. Example:

- `cmake --preset release`
- `cmake --build --preset release`

On Windows the presets commonly generate a Visual Studio solution under the `build/` directory (for example `build/cwipc.sln`).

- Windows (quick):
  1. Run PowerShell as Administrator and install third-party packages: `scripts/install-3rdparty-full-win1064.ps1`.
  2. Create a build dir: `mkdir build && cd build`.
  3. Configure with vcpkg toolchain: `cmake .. -DCMAKE_TOOLCHAIN_FILE=../vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release`.
  4. Build: `cmake --build . --config Release` or open `build/cwipc.sln` in Visual Studio.
  5. Smoke test: `run-cwipc-view-synthetic.bat` or `cwipc_view --synthetic`.
- Linux/macOS: use the scripts in `scripts/install-3rdparty-*.sh` to prepare dependencies, then the same `cmake` configure/build steps. Use `ctest` or the generated test targets (e.g. `RUN_TESTS.vcxproj` on Windows) to run unit tests.

Project-specific conventions and patterns
- CMake options: features like GUI are toggled via CMake options (e.g. `CWIPC_WITH_GUI`). Inspect module `CMakeLists.txt` for target-level `target_include_directories`, `target_link_libraries`, and custom install() rules.
- Public headers live under `include/` and are exported during `install()`; follow the existing pattern when adding APIs.
- Example apps live alongside modules in `*/apps/` — add small CLI demos/tests there rather than scattering examples.
- Packaging: installers and platform-specific behavior are handled via scripts and CMake `install()` tables. Windows runtime dependency copying is done in module CMake files.

Integration points & external dependencies
- vcpkg provides most third-party libraries; avoid adding external system deps without updating `vcpkg.json` and `vcpkg/` ports.
- Unity: native plugin used by `cwipc_unity` (separate repo). Changes that affect the C API must preserve ABI or increment packaging accordingly.
- Python bindings: installed via `cwipc_pymodules_install.sh` / `.bat` and expected to match host Python; CI sometimes relies on those installers.

When making changes
- Prefer small, targeted edits. Run local build and the smoke test (`cwipc_view --synthetic`) to verify runtime changes.
- Update `include/` and `install()` rules when public API surface changes.
- Add CLI examples to the module `apps/` directory and include a brief README there.

References for examples in this repo
- Build script: [scripts/install-3rdparty-full-win1064.ps1](scripts/install-3rdparty-full-win1064.ps1)
- Core library CMake: [cwipc_util/src/CMakeLists.txt](cwipc_util/src/CMakeLists.txt)
- Top-level overview: [readme.md](readme.md)

If anything above is unclear or you'd like more detail (e.g., exact CI/pipeline behaviors or how Python modules are packaged), tell me which area to expand.
