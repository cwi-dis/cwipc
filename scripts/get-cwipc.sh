#!/bin/bash
#
# get-cwipc.sh - get cwipc onto this machine and set it up with setup-cwipc.sh.
# See cwi-dis/cwipc#354.
#
# Usage: get-cwipc.sh [options] VERSION [PREFIX]
#   VERSION         Release tag, for example v8.1.0. With the brew method also
#                   "master", for the current master branch (brew --HEAD).
#   PREFIX          Directory to install into (created if needed). Needed for the
#                   archive method; the brew method installs where brew does.
#   --method M      "archive": download and unpack the -built- release archive
#                   for this machine (default on Linux and Windows).
#                   "brew": build through the brew formula cwi-dis/cwipc/cwipc
#                   (default on macOS, because the macOS archives break when
#                   Homebrew moves on, see cwi-dis/cwipc#356).
#   --archive FILE  Archive method: use this archive instead of downloading it
#                   (for example one just built by CI).
#   --prefix-file F Write the prefix of the result to file F.
#   Other options (--deps, --no-python, --python PY) are passed to setup-cwipc.sh.
#
# Can be fetched and run without a cwipc checkout, for example:
#   curl -sSfL https://raw.githubusercontent.com/cwi-dis/cwipc/<tag or commit>/scripts/get-cwipc.sh -o get-cwipc.sh
#   bash get-cwipc.sh v8.1.0 /opt/cwipc
# With the archive method it then needs the setup script from inside the archive,
# so it only works for releases that include setup-cwipc.sh (8.3.0 and later).
# Run from a checkout (or the install action) it falls back to the setup script
# next to it, which also works for older archives such as v8.1.0.
#
set -euo pipefail

progname=$(basename "$0")
usage() {
    sed -n '3,30p' "$0" | sed 's/^# \{0,1\}//'
}
die() {
    echo "$progname: $*" >&2
    exit 1
}

method=""
archive=""
prefix_file=""
setup_args=()
positional=()
while [ $# -gt 0 ]; do
    case "$1" in
    --method) method="$2"; shift 2 ;;
    --archive) archive="$2"; shift 2 ;;
    --prefix-file) prefix_file="$2"; shift 2 ;;
    --deps|--no-python) setup_args+=("$1"); shift ;;
    --python) setup_args+=("$1" "$2"); shift 2 ;;
    --help|-h) usage; exit 0 ;;
    -*) die "unknown option: $1 (try --help)" ;;
    *) positional+=("$1"); shift ;;
    esac
done
[ ${#positional[@]} -ge 1 ] && [ ${#positional[@]} -le 2 ] || die "need VERSION and PREFIX (try --help)"
version="${positional[0]}"
prefix="${positional[1]:-}"

scriptdir=$(cd "$(dirname "$0")" && pwd)

case "$(uname -s)/$(uname -m)" in
Linux/x86_64)
    platform=ubuntu2404-amd64; ext=tar.gz
    if [ -f /etc/os-release ] && ! grep -q 'VERSION_ID="24.04"' /etc/os-release; then
        echo "$progname: warning: the Linux archive is built for Ubuntu 24.04" >&2
    fi
    ;;
Darwin/arm64) platform=macos-arm64; ext=tar.gz ;;
Darwin/x86_64) platform=macos-intel; ext=tar.gz ;;
MINGW*/x86_64|MSYS*/x86_64|CYGWIN*/x86_64) platform=windows-intel; ext=zip ;;
*) die "no cwipc for $(uname -s)/$(uname -m)" ;;
esac
if [ -z "$method" ]; then
    case "$platform" in
    macos-*) method=brew ;;
    *) method=archive ;;
    esac
fi

# Find the setup script: from the tree itself if it has one (it matches the
# tree's version), else the one next to us.
find_setup() {
    local setup="$1/libexec/cwipc/scripts/setup-cwipc.sh"
    if [ ! -f "$setup" ]; then
        setup="$scriptdir/setup-cwipc.sh"
        [ -f "$setup" ] || die "the tree has no setup-cwipc.sh (older than 8.3.0?), and there is none next to $progname"
        echo "$progname: tree has no setup script, using $setup" >&2
    fi
    echo "$setup"
}

case "$method" in
archive)
    [ -n "$prefix" ] || die "the archive method needs a PREFIX"
    asset="cwipc-$platform-built-$version.$ext"
    tmpdir=$(mktemp -d)
    trap 'rm -rf "$tmpdir"' EXIT
    if [ -z "$archive" ]; then
        url="https://github.com/cwi-dis/cwipc/releases/download/$version/$asset"
        echo "$progname: downloading $url"
        archive="$tmpdir/$asset"
        curl -sSfL -o "$archive" "$url" || die "download failed: $url"
    fi
    [ -f "$archive" ] || die "no such archive: $archive"
    # The tar archives have the tree at the top; the Windows zip has it under installed/.
    mkdir -p "$prefix"
    prefix=$(cd "$prefix" && pwd)
    echo "$progname: unpacking into $prefix"
    case "$archive" in
    *.tar.gz)
        tar xzf "$archive" -C "$prefix"
        ;;
    *.zip)
        if command -v unzip >/dev/null 2>&1; then
            unzip -q "$archive" -d "$tmpdir/unzipped"
        else
            powershell -NoProfile -Command "Expand-Archive -Path '$(cygpath -w "$archive")' -DestinationPath '$(cygpath -w "$tmpdir/unzipped")'"
        fi
        if [ -d "$tmpdir/unzipped/installed" ]; then
            cp -R "$tmpdir/unzipped/installed/." "$prefix/"
        else
            cp -R "$tmpdir/unzipped/." "$prefix/"
        fi
        ;;
    *) die "unknown archive type: $archive" ;;
    esac
    setup=$(find_setup "$prefix")
    bash "$setup" --prefix "$prefix" ${setup_args[@]+"${setup_args[@]}"}
    ;;
brew)
    command -v brew >/dev/null 2>&1 || die "the brew method needs Homebrew"
    [ -z "$archive" ] || die "--archive can't be used with the brew method"
    [ -z "$prefix" ] || echo "$progname: brew method: ignoring PREFIX $prefix, brew decides where cwipc goes" >&2
    # No "brew tap": installing the fully qualified cwi-dis/cwipc/cwipc taps by itself
    # and trusts only that formula (Homebrew 6 tap trust, https://docs.brew.sh/Tap-Trust).
    # "brew install" exits with an error when a *dependency* can't be linked, even if
    # cwipc itself installed fine. That happens on GitHub's Intel runners, where
    # python.org's Python 3.12 already owns /usr/local/bin/python3.12 and brew won't
    # link python@3.12 over it. So judge by whether cwipc got installed, without
    # overwriting anything (no "brew link --overwrite" on someone's own Mac).
    brew_install() {
        if ! brew install "$@"; then
            brew list --versions cwipc >/dev/null 2>&1 || die "brew install $* failed"
            echo "$progname: warning: brew install reported an error, but cwipc is installed; continuing" >&2
        fi
    }
    if [ "$version" = master ]; then
        # Workaround (from lldash): git-lfs must be findable by git when brew clones --HEAD
        brew install git-lfs
        lfs_wtd="$(git --exec-path)/git-lfs"
        [ -e "$lfs_wtd" ] || ln -s "$(command -v git-lfs)" "$lfs_wtd" || true
        brew_install --HEAD cwi-dis/cwipc/cwipc
    else
        brew_install cwi-dis/cwipc/cwipc
        installed=$(brew list --versions cwipc | awk '{print $2}')
        if [ "v$installed" != "$version" ]; then
            echo "$progname: warning: asked for $version, but the brew formula installed $installed" >&2
        fi
    fi
    prefix=$(brew --prefix cwipc)
    setup=$(find_setup "$prefix")
    # A brew install already has its venv with the cwipc modules, and its dependencies.
    brew_args=()
    for a in ${setup_args[@]+"${setup_args[@]}"}; do
        case "$a" in
        --deps) ;;
        *) brew_args+=("$a") ;;
        esac
    done
    case " ${brew_args[*]-} " in
    *" --no-python "*) ;;
    *) brew_args+=(--existing-venv) ;;
    esac
    bash "$setup" --prefix "$prefix" ${brew_args[@]+"${brew_args[@]}"}
    ;;
*) die "unknown method: $method (archive or brew)" ;;
esac

if [ -n "$prefix_file" ]; then
    echo "$prefix" > "$prefix_file"
fi
echo "$progname: cwipc is in $prefix"
if [ -f "$prefix/libexec/cwipc/activate" ]; then
    echo "$progname: to use it: source $prefix/libexec/cwipc/activate"
fi
