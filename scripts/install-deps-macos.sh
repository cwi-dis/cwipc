#!/bin/bash
#
# install-deps-macos.sh - install cwipc's dependencies on macOS with Homebrew.
#
# Usage: install-deps-macos.sh [--runtime | --build]
#   --runtime   What an installed cwipc tree needs to run (default).
#   --build     Also what is needed to build cwipc from source, including OrbbecSDK
#               (into <repo>/3rdparty_installed; only meaningful in a source tree).
#
# Python packages are not installed here: setup-cwipc.sh installs them into the venv.
# What goes in which level, and why: cwi-dis/cwipc#355 (decision of 2026-11-13).
# Versions: versions.env.
#
set -euo pipefail
scriptdir=$(cd "$(dirname "$0")" && pwd)
source "$scriptdir/versions.env"

level=runtime
case "${1:-}" in
"" | --runtime) level=runtime ;;
--build) level=build ;;
--help | -h) sed -n '3,13p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
*) echo "$(basename "$0"): unknown argument: $1 (try --help)" >&2; exit 1 ;;
esac

command -v brew >/dev/null 2>&1 || { echo "$(basename "$0"): Homebrew not found, see https://brew.sh" >&2; exit 1; }

# pcl brings boost, flann and lz4 (and vtk/qt, which cwipc doesn't use, cwi-dis/cwipc#357).
runtime_packages=(pcl libomp opencv jpeg-turbo glfw librealsense "python@$PYTHON_VERSION")
build_packages=(cmake pkg-config nlohmann-json git-lfs)

packages=("${runtime_packages[@]}")
if [ "$level" = build ]; then
    packages+=("${build_packages[@]}")
fi
echo "$(basename "$0"): installing ($level level): ${packages[*]}"
brew install "${packages[@]}"

if [ "$level" = build ]; then
    bash "$scriptdir/install-orbbecsdk-macos.sh"
fi
