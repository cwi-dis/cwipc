#!/bin/bash
#
# install-deps-ubuntu2404.sh - install cwipc's dependencies on Ubuntu 24.04 with apt.
#
# Usage: install-deps-ubuntu2404.sh [--runtime | --build] [--no-orbbec-viewer]
#   --runtime   What an installed cwipc tree needs to run (default).
#   --build     Also what is needed to build cwipc from source, including OrbbecSDK
#               and OrbbecViewer (into <repo>/3rdparty_installed, with
#               install-orbbecsdk.sh; only meaningful in a source tree).
#   --no-orbbec-viewer  With --build: without OrbbecViewer.
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
orbbec_viewer=--viewer
while [ $# -gt 0 ]; do
    case "$1" in
    --runtime) level=runtime ;;
    --build) level=build ;;
    --no-orbbec-viewer) orbbec_viewer="" ;;
    --help | -h) sed -n '3,17p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) echo "$(basename "$0"): unknown argument: $1 (try --help)" >&2; exit 1 ;;
    esac
    shift
done

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
    bash "$scriptdir/install-orbbecsdk.sh" $orbbec_viewer
fi

# The udev rules a USB Orbbec camera needs to be usable by non-root users
# (cwi-dis/cwipc#351), into /etc/udev/rules.d. From an installed tree
# (<prefix>/lib/udev/rules.d, not needed for the Debian package, which has them in
# /usr/lib/udev/rules.d already) or a source tree (the OrbbecSDK in 3rdparty_installed).
rules=99-obsensor-libusb.rules
tree=$(cd "$scriptdir/../../.." 2>/dev/null && pwd || true)
repo=$(dirname "$scriptdir")
rulesfile=""
for candidate in "$tree/lib/udev/rules.d/$rules" "$repo/3rdparty_installed/orbbecsdk/shared/$rules"; do
    if [ -f "$candidate" ]; then
        rulesfile="$candidate"
        break
    fi
done
if [ "$rulesfile" = "/usr/lib/udev/rules.d/$rules" ]; then
    : # the Debian package's own
elif [ -n "$rulesfile" ]; then
    if ! cmp -s "$rulesfile" "/etc/udev/rules.d/$rules"; then
        echo "$(basename "$0"): installing the udev rules for Orbbec cameras: /etc/udev/rules.d/$rules"
        sudo install -m 644 "$rulesfile" "/etc/udev/rules.d/$rules"
        if command -v udevadm >/dev/null 2>&1 && [ -d /run/udev ]; then
            sudo udevadm control --reload-rules || true
            sudo udevadm trigger --subsystem-match=usb || true
        fi
    fi
else
    echo "$(basename "$0"): no udev rules for Orbbec cameras found in this tree; USB Orbbec cameras will need root"
fi
