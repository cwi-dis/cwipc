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

runtime_packages=(
    python3-venv
    libglfw3 libglu1-mesa libopengl0 libegl1 libgomp1
    libturbojpeg libjpeg8 liblz4-1
    libopencv-core406t64 libopencv-imgproc406t64
    libpcl-common1.14 libpcl-octree1.14 libpcl-filters1.14 libpcl-features1.14
    libpcl-search1.14 libpcl-kdtree1.14 libpcl-sample-consensus1.14
    librealsense2
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
