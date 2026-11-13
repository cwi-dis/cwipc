#!/bin/bash
#
# setup-cwipc.sh - set up a cwipc tree so it can be used: optionally install
# dependencies, create a Python venv with the cwipc modules, and write the
# activate file. See cwi-dis/cwipc#354 for the design.
#
# Usage: setup-cwipc.sh [options]
#   --prefix DIR   The tree to set up. Default: found from this script's location:
#                  <prefix>/libexec/cwipc/scripts/setup-cwipc.sh (installed tree), or
#                  <repo>/scripts/setup-cwipc.sh (source tree, prefix <repo>/build).
#   --deps         Also install dependencies (system-wide: uses sudo/brew).
#   --no-python    Don't create the venv or install the Python modules.
#   --existing-venv  The tree already has its venv with the cwipc modules (a brew
#                  install): use it as it is.
#   --editable     Install the Python modules editable from the source tree
#                  (default in a source tree).
#   --wheels       Install the Python modules from the wheels in
#                  <prefix>/share/cwipc/python (default in an installed tree).
#   --config CFG   Windows source tree: use build/bin/CFG. Default: the first of
#                  Debug, RelWithDebInfo, Release that exists.
#   --python PY    Python to create the venv with. Default: python3.12 (or python3)
#                  on Linux/macOS, python on Windows.
#   --help         Show this help.
#
# Afterwards:
#   source <prefix>/activate              (bash/zsh, Git Bash)
#   <prefix>\activate.ps1                 (PowerShell, Windows)
# and <prefix>/libexec/cwipc/environment holds the same settings in a simple
# machine-readable form, used by the install action (.github/actions/install-cwipc):
#   path DIR            prepend DIR to PATH
#   env NAME=VALUE      set NAME
#   libpath NAME DIR    prepend DIR to the library path variable NAME
#
# In a source tree the venv is normally created by CMake (at configure time, for
# building the wheels and for the Python tests); this script reuses it and never
# recreates it there.
#
set -euo pipefail

progname=$(basename "$0")
usage() {
    sed -n '3,37p' "$0" | sed 's/^# \{0,1\}//'
}
die() {
    echo "$progname: $*" >&2
    exit 1
}

prefix=""
deps=false
python_modules=true
existing_venv=false
modules=""      # editable or wheels; default from the context
config=""
python=""
while [ $# -gt 0 ]; do
    case "$1" in
    --prefix) prefix="$2"; shift 2 ;;
    --deps) deps=true; shift ;;
    --no-python) python_modules=false; shift ;;
    --existing-venv) existing_venv=true; shift ;;
    --editable) modules=editable; shift ;;
    --wheels) modules=wheels; shift ;;
    --config) config="$2"; shift 2 ;;
    --python) python="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) die "unknown argument: $1 (try --help)" ;;
    esac
done

scriptdir=$(cd "$(dirname "$0")" && pwd)

#
# Platform
#
case "$(uname -s)" in
Darwin) platform=macos ;;
Linux) platform=linux ;;
MINGW*|MSYS*|CYGWIN*) platform=windows ;;
*) die "unsupported platform: $(uname -s)" ;;
esac

#
# Find the prefix.
#
# Source tree: this script is <repo>/scripts/setup-cwipc.sh. Also used to find the
# Python package sources for editable installs.
srcdir=""
if [ -f "$scriptdir/../CMakeLists.txt" ] && [ -d "$scriptdir/../cwipc_util" ]; then
    srcdir=$(cd "$scriptdir/.." && pwd)
fi
if [ -z "$prefix" ]; then
    case "$scriptdir" in
    */libexec/cwipc/scripts)
        prefix=$(cd "$scriptdir/../../.." && pwd)
        ;;
    *)
        [ -n "$srcdir" ] || die "cannot determine the cwipc tree from the script location $scriptdir. Use --prefix."
        prefix="$srcdir/build"
        [ -d "$prefix" ] || die "no build directory $prefix: configure and build cwipc first (see docs/build-from-source.rst)"
        ;;
    esac
fi
[ -d "$prefix" ] || die "no such directory: $prefix"
prefix=$(cd "$prefix" && pwd)
[ -d "$prefix/bin" ] || die "$prefix does not look like a cwipc tree (no bin/)"
context=installed
if [ -n "$srcdir" ] && [ "$prefix" = "$srcdir/build" ]; then
    context=source
fi
if [ -z "$modules" ]; then
    if [ "$context" = source ]; then modules=editable; else modules=wheels; fi
fi
if [ "$modules" = editable ] && [ -z "$srcdir" ]; then
    die "--editable needs a source tree (run the copy of this script in <repo>/scripts)"
fi

# Executables (and on Windows the DLLs). A Windows source tree has bin/<Config>.
bindir="$prefix/bin"
if [ "$platform" = windows ] && [ "$context" = source ]; then
    if [ -n "$config" ]; then
        [ -d "$bindir/$config" ] || die "no such configuration: $bindir/$config"
        bindir="$bindir/$config"
    else
        for c in Debug RelWithDebInfo Release; do
            if [ -d "$bindir/$c" ]; then
                bindir="$bindir/$c"
                break
            fi
        done
    fi
fi
if [ "$platform" = windows ]; then
    libdir="$bindir"
else
    libdir="$prefix/lib"
fi
wheeldir="$prefix/share/cwipc/python"
venv="$prefix/libexec/cwipc/venv"
envfile="$prefix/libexec/cwipc/environment"

# Native path for files read by non-bash consumers (GitHub, PowerShell, Python)
native() {
    if [ "$platform" = windows ]; then
        cygpath -w "$1"
    else
        echo "$1"
    fi
}

#
# Dependencies. Phase 0 uses the existing install-3rdparty scripts, preferring
# the copy in the tree (matching its version) over our own.
#
if $deps; then
    case "$platform" in
    linux) depscript=install-3rdparty-ubuntu2404.sh ;;
    macos) depscript=install-3rdparty-macos.sh ;;
    windows) depscript="" ;;  # the Windows archive carries its DLLs in bin/
    esac
    if [ -n "$depscript" ]; then
        if [ -f "$prefix/libexec/cwipc/scripts/$depscript" ]; then
            depscript="$prefix/libexec/cwipc/scripts/$depscript"
        else
            depscript="$scriptdir/$depscript"
        fi
        echo "$progname: installing dependencies with $depscript"
        bash "$depscript"
    fi
fi

#
# Python venv with the cwipc modules.
#
if [ "$platform" = windows ]; then
    venvbin="$venv/Scripts"
    venvpython="$venvbin/python.exe"
else
    venvbin="$venv/bin"
    venvpython="$venvbin/python"
fi
if $python_modules && $existing_venv; then
    [ -x "$venvpython" ] || die "--existing-venv, but there is no venv in $venv"
    echo "$progname: using the existing venv $venv"
elif $python_modules; then
    if [ "$context" = source ] && [ -x "$venvpython" ]; then
        # Created by CMake, which also uses it to build the wheels: don't recreate.
        echo "$progname: using the venv $venv"
    else
        if [ -z "$python" ]; then
            if [ "$platform" = windows ]; then
                python=python
            elif command -v python3.12 >/dev/null 2>&1; then
                python=python3.12
            else
                python=python3
            fi
        fi
        command -v "$python" >/dev/null 2>&1 || die "Python not found: $python (use --python)"
        echo "$progname: creating venv $venv with $("$python" --version 2>&1)"
        mkdir -p "$(dirname "$venv")"
        "$python" -m venv --clear "$(native "$venv")"
        "$venvpython" -m pip install --quiet --upgrade pip
    fi
    if [ "$modules" = editable ]; then
        echo "$progname: installing the cwipc Python modules editable from $srcdir"
        packages=()
        for m in cwipc_util cwipc_codec cwipc_realsense2 cwipc_kinect cwipc_orbbec; do
            packages+=(--editable "$(native "$srcdir/$m/python")")
        done
        "$venvpython" -m pip install --quiet "${packages[@]}"
    else
        ls "$wheeldir"/cwipc_*.whl >/dev/null 2>&1 || die "no cwipc wheels in $wheeldir"
        echo "$progname: installing the cwipc Python modules from $wheeldir"
        wheels=()
        for w in "$wheeldir"/cwipc_*.whl; do
            wheels+=("$(native "$w")")
        done
        "$venvpython" -m pip install --quiet --upgrade --find-links="$(native "$wheeldir")" "${wheels[@]}"
    fi
fi

#
# Environment file, activate file (sh) and activate.ps1 (Windows).
#
mkdir -p "$(dirname "$envfile")"
{
    echo "# Generated by $progname. Format: see the comment at the top of setup-cwipc.sh."
    echo "path $(native "$bindir")"
    if $python_modules; then
        echo "path $(native "$venvbin")"
        echo "env VIRTUAL_ENV=$(native "$venv")"
    fi
    echo "env CWIPC_LIBRARY_DIR=$(native "$libdir")"
    case "$platform" in
    linux) echo "libpath LD_LIBRARY_PATH $libdir" ;;
    macos) echo "libpath DYLD_LIBRARY_PATH $libdir" ;;
    esac
} > "$envfile"

{
    echo "# Generated by $progname: use with \"source\", don't execute."
    echo "export PATH=\"$bindir:\$PATH\""
    echo "export CWIPC_LIBRARY_DIR=\"$(native "$libdir")\""
    case "$platform" in
    linux) echo "export LD_LIBRARY_PATH=\"$libdir\${LD_LIBRARY_PATH:+:\$LD_LIBRARY_PATH}\"" ;;
    macos) echo "export DYLD_LIBRARY_PATH=\"$libdir\${DYLD_LIBRARY_PATH:+:\$DYLD_LIBRARY_PATH}\"" ;;
    esac
    if $python_modules; then
        echo "source \"$venvbin/activate\""
    fi
} > "$prefix/activate"

if [ "$platform" = windows ]; then
    {
        echo "# Generated by $progname: run in PowerShell to set up the environment."
        echo "\$Env:Path = \"$(native "$bindir");\" + \$Env:Path"
        echo "\$Env:CWIPC_LIBRARY_DIR = \"$(native "$libdir")\""
        if $python_modules; then
            echo "& \"$(native "$venvbin/Activate.ps1")\""
        fi
    } > "$prefix/activate.ps1"
fi

echo "$progname: done. Use: source $prefix/activate"
