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
#   --deps         Also install dependencies (system-wide: uses sudo/brew), with
#                  install-deps-<platform>: the build level in a source tree, the
#                  runtime level in an installed tree.
#   --deps-level L Level for --deps: runtime or build.
#   --no-python    Don't create the venv or install the Python modules.
#   --existing-venv  The tree already has its venv with the cwipc modules (a brew
#                  install): use it as it is.
#   --editable     Install the Python modules editable from the source tree
#                  (default in a source tree).
#   --wheels       Install the Python modules from the wheels in
#                  <prefix>/share/cwipc/python (default in an installed tree).
#   --config CFG   Windows source tree: use build/bin/CFG. Default: the first of
#                  Debug, RelWithDebInfo, Release that exists.
#   --python PY    Python to create the venv with. Must be between PYTHON_VERSION_MIN
#                  and PYTHON_VERSION from versions.env. Default: the first suitable
#                  of python<PYTHON_VERSION>, python<PYTHON_VERSION_MIN>, python3
#                  on Linux/macOS; python on Windows.
#   --launchers    Also put the Python launchers (cwipc, cwipc_view, ...) in
#                  <prefix>/bin: copies on Windows, symlinks elsewhere. For system
#                  installations (Windows installer, Debian package), so cwipc works
#                  without activating anything. The list is kept in
#                  <prefix>/libexec/cwipc/launchers, for uninstalling.
#   --help         Show this help.
#
# Writes these per-tree files in <prefix>/libexec/cwipc/ (cwi-dis/cwipc#355):
#   activate       source it (bash/zsh, Git Bash) to get PATH, the library path,
#                  CWIPC_LIBRARY_DIR and the venv; activate.ps1 for PowerShell
#   environment    the same settings in a simple machine-readable form, used by the
#                  install action (.github/actions/install-cwipc):
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
    sed -n '3,50p' "$0" | sed 's/^# \{0,1\}//'
}
die() {
    echo "$progname: $*" >&2
    exit 1
}

prefix=""
deps=false
deps_level=""
launchers=false
python_modules=true
existing_venv=false
modules=""      # editable or wheels; default from the context
config=""
python=""
while [ $# -gt 0 ]; do
    case "$1" in
    --prefix) prefix="$2"; shift 2 ;;
    --deps) deps=true; shift ;;
    --deps-level) deps=true; deps_level="$2"; shift 2 ;;
    --no-python) python_modules=false; shift ;;
    --existing-venv) existing_venv=true; shift ;;
    --editable) modules=editable; shift ;;
    --wheels) modules=wheels; shift ;;
    --config) config="$2"; shift 2 ;;
    --launchers) launchers=true; shift ;;
    --python) python="$2"; shift 2 ;;
    --help|-h) usage; exit 0 ;;
    *) die "unknown argument: $1 (try --help)" ;;
    esac
done

scriptdir=$(cd "$(dirname "$0")" && pwd)
# Pinned versions (PYTHON_VERSION, ...), cwi-dis/cwipc#355
source "$scriptdir/versions.env"

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
# Dependencies, with install-deps-<platform> (cwi-dis/cwipc#355). Use the tree's own
# script, because it matches the tree's version: a tree from before 8.3.0 has no
# install-deps-*, but its install-3rdparty-* installs what that tree needs (for
# example libpcl_io, which cwipc no longer links since 8.3.0, cwi-dis/cwipc#357).
# Only without either do we use our own copy.
#
if $deps; then
    if [ -z "$deps_level" ]; then
        if [ "$context" = source ]; then deps_level=build; else deps_level=runtime; fi
    fi
    case "$deps_level" in
    runtime|build) ;;
    *) die "--deps-level must be runtime or build, not $deps_level" ;;
    esac
    case "$platform" in
    linux) depscript=install-deps-ubuntu2404.sh; legacyscript=install-3rdparty-ubuntu2404.sh ;;
    macos) depscript=install-deps-macos.sh; legacyscript=install-3rdparty-macos.sh ;;
    windows) depscript=install-deps-windows.ps1; legacyscript="" ;;  # the native DLLs come with the tree; this checks Python (and the build tools)
    esac
    treescripts="$prefix/libexec/cwipc/scripts"
    if [ -f "$treescripts/$depscript" ]; then
        depscript="$treescripts/$depscript"
    elif [ -n "$legacyscript" ] && [ -f "$treescripts/$legacyscript" ]; then
        echo "$progname: tree from before 8.3.0: installing its dependencies with $treescripts/$legacyscript"
        bash "$treescripts/$legacyscript"
        depscript=""
    else
        depscript="$scriptdir/$depscript"
    fi
    if [ -z "$depscript" ]; then
        :
    elif [ "$platform" = windows ]; then
        echo "$progname: installing dependencies ($deps_level level) with $depscript"
        if [ "$deps_level" = build ]; then level_arg=-Build; else level_arg=-Runtime; fi
        powershell.exe -NoProfile -ExecutionPolicy Bypass -File "$(native "$depscript")" "$level_arg"
    else
        echo "$progname: installing dependencies ($deps_level level) with $depscript"
        bash "$depscript" "--$deps_level"
    fi
fi

#
# Python venv with the cwipc modules.
#
python_version() {
    "$1" -c 'import sys; print("%d.%d" % sys.version_info[:2])' 2>/dev/null || echo unknown
}
# Is Python $1 between PYTHON_VERSION_MIN and PYTHON_VERSION (minor versions)?
python_version_ok() {
    local v
    v=$(python_version "$1")
    [ "$v" != unknown ] || return 1
    [ "$(printf '%s\n%s\n' "$PYTHON_VERSION_MIN" "$v" | sort -t. -k1,1n -k2,2n | head -1)" = "$PYTHON_VERSION_MIN" ] &&
    [ "$(printf '%s\n%s\n' "$v" "$PYTHON_VERSION" | sort -t. -k1,1n -k2,2n | head -1)" = "$v" ]
}
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
        # Accepted: PYTHON_VERSION_MIN up to and including PYTHON_VERSION (versions.env,
        # cwi-dis/cwipc#355). The preferred (newest) one first.
        pyrange="$PYTHON_VERSION_MIN to $PYTHON_VERSION"
        [ "$PYTHON_VERSION_MIN" = "$PYTHON_VERSION" ] && pyrange="$PYTHON_VERSION"
        if [ -z "$python" ]; then
            if [ "$platform" = windows ]; then
                candidates=(python)
            else
                candidates=("python$PYTHON_VERSION" "python$PYTHON_VERSION_MIN" python3)
            fi
            for c in "${candidates[@]}"; do
                if command -v "$c" >/dev/null 2>&1 && python_version_ok "$c"; then
                    python="$c"
                    break
                fi
            done
            [ -n "$python" ] || die "no Python $pyrange found (use --deps to install it, or --python)"
        else
            command -v "$python" >/dev/null 2>&1 || die "Python not found: $python"
            python_version_ok "$python" || die "Python $pyrange needed, but $python is $(python_version "$python")"
        fi
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
} > "$prefix/libexec/cwipc/activate"

if [ "$platform" = windows ]; then
    {
        echo "# Generated by $progname: run in PowerShell to set up the environment."
        echo "\$Env:Path = \"$(native "$bindir");\" + \$Env:Path"
        echo "\$Env:CWIPC_LIBRARY_DIR = \"$(native "$libdir")\""
        if $python_modules; then
            echo "& \"$(native "$venvbin/Activate.ps1")\""
        fi
    } > "$prefix/libexec/cwipc/activate.ps1"
fi

# Before cwi-dis/cwipc#355 the activate files were at the top of the prefix.
for f in "$prefix/activate" "$prefix/activate.ps1"; do
    if [ -f "$f" ] && head -1 "$f" | grep -q "^# Generated by $progname"; then
        rm -f "$f"
    fi
done

#
# Launchers in <prefix>/bin.
#
if $launchers && $python_modules; then
    launcherlist="$prefix/libexec/cwipc/launchers"
    # Remove the ones we placed before (the set of launchers may have changed).
    if [ -f "$launcherlist" ]; then
        while read -r name; do
            [ -n "$name" ] && rm -f "$bindir/$name"
        done < "$launcherlist"
    fi
    : > "$launcherlist"
    if [ "$platform" = windows ]; then
        pattern="cwipc*.exe"
    else
        pattern="cwipc*"
    fi
    for launcher in "$venvbin"/$pattern; do
        [ -f "$launcher" ] || continue
        name=$(basename "$launcher")
        if [ -e "$bindir/$name" ] || [ -L "$bindir/$name" ]; then
            echo "$progname: warning: not replacing $bindir/$name with the launcher from the venv" >&2
            continue
        fi
        if [ "$platform" = windows ]; then
            cp "$launcher" "$bindir/$name"
        else
            ln -s "$launcher" "$bindir/$name"
        fi
        echo "$name" >> "$launcherlist"
    done
    echo "$progname: launchers in $bindir: $(tr '\n' ' ' < "$launcherlist")"
fi

if [ "$context" = source ]; then
    echo "$progname: done. Use: source $srcdir/scripts/activate"
else
    echo "$progname: done. Use: source $prefix/libexec/cwipc/activate"
fi
