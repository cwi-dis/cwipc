#!/bin/bash
#
# install-deps-ubuntu2404.sh - install cwipc's dependencies on Ubuntu 24.04 with apt.
#
# Usage: install-deps-ubuntu2404.sh [--runtime | --build]
#   --runtime   What an installed cwipc tree needs to run (default).
#   --build     Also what is needed to build cwipc from source, including OrbbecSDK
#               (into <repo>/3rdparty_installed; only meaningful in a source tree).
#
# Uses sudo. Adds the RealSense apt repository (librealsense.realsenseai.com) if it
# isn't there yet. No "apt-get upgrade": that made CI slow and unpredictable.
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
--help | -h) sed -n '3,15p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
*) echo "$(basename "$0"): unknown argument: $1 (try --help)" >&2; exit 1 ;;
esac

export DEBIAN_FRONTEND=noninteractive

# The RealSense apt repository.
realsense_key=/etc/apt/keyrings/librealsenseai.gpg
realsense_list=/etc/apt/sources.list.d/librealsense.list
if [ ! -f "$realsense_key" ] || [ ! -f "$realsense_list" ]; then
    echo "$(basename "$0"): adding the RealSense apt repository"
    sudo apt-get update
    sudo apt-get install -y curl gpg lsb-release ca-certificates
    sudo mkdir -p /etc/apt/keyrings
    curl -sSf https://librealsense.realsenseai.com/Debian/librealsenseai.asc | gpg --dearmor | sudo tee "$realsense_key" > /dev/null
    echo "deb [signed-by=$realsense_key] https://librealsense.realsenseai.com/Debian/apt-repo $(lsb_release -cs) main" | sudo tee "$realsense_list" > /dev/null
fi

# The same as the Debian package's dependencies (CMakeFiles/CwipcInstallers.cmake), plus
# librealsense2: what cwipc's own libraries need (dpkg-shlibdeps; Ubuntu links with
# --as-needed, so of PCL only libpcl-common is needed at runtime), and what the pip
# wheels in the venv need (open3d, PySide6/Qt, ...; procedure: docs/distribution.rst).
runtime_packages=(
    python3-venv
    libglfw3 libglu1-mesa libopengl0 libgomp1 libjpeg8 liblz4-1
    libpcl-common1.14
    librealsense2
    libdbus-1-3 libegl1 libfontconfig1 libglib2.0-0t64 libidn2-0 libusb-1.0-0
    libwayland-cursor0 libxcb-cursor0 libxcb-icccm4 libxcb-image0 libxcb-keysyms1
    libxcb-render-util0 libxcb-shape0 libxkbcommon-x11-0
)
build_packages=(
    build-essential cmake git git-lfs pkg-config curl unzip
    libpcl-dev libturbojpeg0-dev libglfw3-dev libopencv-dev libusb-1.0-0-dev
    librealsense2-dev
)

packages=("${runtime_packages[@]}")
if [ "$level" = build ]; then
    packages+=("${build_packages[@]}")
fi
echo "$(basename "$0"): installing ($level level): ${packages[*]}"
sudo apt-get update
sudo apt-get install -y "${packages[@]}"

if [ "$level" = build ]; then
    bash "$scriptdir/install-orbbecsdk-ubuntu2404.sh"
fi
