#!/bin/bash
#
# install-orbbecsdk.sh - install the OrbbecSDK, and optionally its viewer, into
# <repo>/3rdparty_installed, for building cwipc_orbbec from source (macOS, Linux).
#
# Usage: install-orbbecsdk.sh [--viewer]
#   --viewer    Also install OrbbecViewer (for development: seeing what a camera
#               does without cwipc). There is none for Intel Macs.
#
# Result: 3rdparty_installed/orbbecsdk (where CMake looks for the SDK) and
# 3rdparty_installed/orbbecviewer, each a link to the unpacked release. Skips what
# is already there in the version from versions.env. Installs nothing system-wide:
# the udev rules a USB camera needs on Linux come with the cwipc Debian package and
# the runtime level of install-deps-ubuntu2404.sh.
# Versions and archive names: versions.env (cwi-dis/cwipc#351). Called by
# install-deps-macos.sh and install-deps-ubuntu2404.sh at the build level.
#
set -euo pipefail
progname=$(basename "$0")
scriptdir=$(cd "$(dirname "$0")" && pwd)
source "$scriptdir/versions.env"
die() {
    echo "$progname: $*" >&2
    exit 1
}

viewer=false
while [ $# -gt 0 ]; do
    case "$1" in
    --viewer) viewer=true ;;
    --help | -h) sed -n '3,17p' "$0" | sed 's/^# \{0,1\}//'; exit 0 ;;
    *) die "unknown argument: $1 (try --help)" ;;
    esac
    shift
done

case "$(uname -s)/$(uname -m)" in
Darwin/arm64) sdk_archive="$ORBBECSDK_ARCHIVE_MACOS"; viewer_archive="$ORBBECVIEWER_ARCHIVE_MACOS" ;;
Darwin/x86_64) sdk_archive="$ORBBECSDK_ARCHIVE_MACOS"; viewer_archive="" ;;
Linux/x86_64) sdk_archive="$ORBBECSDK_ARCHIVE_LINUX"; viewer_archive="$ORBBECVIEWER_ARCHIVE_LINUX" ;;
*) die "no OrbbecSDK for $(uname -s)/$(uname -m) in versions.env" ;;
esac

dest="$(dirname "$scriptdir")/3rdparty_installed"
mkdir -p "$dest"
url_base="https://github.com/orbbec/OrbbecSDK_v2/releases/download/v$ORBBECSDK_VERSION"

# Temporary directories, removed when the script ends, also on failure.
tmpdirs=()
cleanup() {
    [ ${#tmpdirs[@]} -eq 0 ] || rm -rf "${tmpdirs[@]}"
}
trap cleanup EXIT

# install_archive ARCHIVE LINK: unpack ARCHIVE into 3rdparty_installed/<archive name
# without extension> and point LINK at it.
install_archive() {
    local archive="$1" link="$2"
    local name="${archive%.tar.gz}"
    name="${name%.zip}"
    if [ "$(readlink "$dest/$link" 2>/dev/null)" = "$name" ] && [ -d "$dest/$name" ]; then
        echo "$progname: $name already installed"
        return
    fi
    local tmp
    tmp=$(mktemp -d "$dest/.download.XXXXXX")
    tmpdirs+=("$tmp")
    echo "$progname: downloading $archive"
    curl -sSfL -o "$tmp/$archive" "$url_base/$archive" || die "download failed: $url_base/$archive"
    mkdir "$tmp/unpacked"
    case "$archive" in
    *.tar.gz) tar xzf "$tmp/$archive" -C "$tmp/unpacked" ;;
    *.zip) unzip -q "$tmp/$archive" -d "$tmp/unpacked" ;;
    *) die "unknown archive type: $archive" ;;
    esac
    # Most archives have everything under one directory; some have it at the top.
    local content="$tmp/unpacked"
    local entries=("$tmp/unpacked"/*)
    if [ ${#entries[@]} -eq 1 ] && [ -d "${entries[0]}" ]; then
        content="${entries[0]}"
    fi
    rm -rf "${dest:?}/$name"
    mv "$content" "$dest/$name"
    # A downloaded archive may carry the quarantine attribute (what the SDK's own
    # setup.sh removes too).
    if [ "$(uname -s)" = Darwin ]; then
        xattr -dr com.apple.quarantine "$dest/$name" 2>/dev/null || true
    fi
    ln -sfn "$name" "$dest/$link"
    echo "$progname: installed $dest/$link -> $name"
}

install_archive "$sdk_archive" orbbecsdk
if $viewer; then
    if [ -n "$viewer_archive" ]; then
        install_archive "$viewer_archive" orbbecviewer
    else
        echo "$progname: there is no OrbbecViewer for $(uname -s)/$(uname -m), skipped"
    fi
fi
