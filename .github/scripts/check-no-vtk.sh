#!/bin/bash
#
# check-no-vtk.sh PREFIX - check that no cwipc library or program in the installed
# tree PREFIX links VTK, PCL's io library or Qt (cwi-dis/cwipc#357), and on Windows
# that none of their DLLs were installed alongside.
# Exit status 1 if any were found. Used by the CI build jobs (cwi-dis/cwipc#355).
#
set -euo pipefail
prefix="${1:?usage: $0 PREFIX}"
pattern='vtk|pcl_io|(^|[/\\[:space:]])(lib)?qt[0-9a-z]*(core|gui)'
found=0

check() {   # check NAME DEPENDENCY-LIST
    local hits
    hits=$(echo "$2" | grep -i -E "$pattern" || true)
    if [ -n "$hits" ]; then
        echo "FOUND in $1:"
        echo "$hits" | sed 's/^/    /'
        found=1
    fi
}

case "$(uname -s)" in
Darwin)
    for f in "$prefix"/lib/libcwipc_*.dylib "$prefix"/bin/cwipc_*; do
        [ -f "$f" ] && [ ! -L "$f" ] && file "$f" | grep -q Mach-O || continue
        check "$f" "$(otool -L "$f" | tail -n +2)"
    done
    ;;
Linux)
    for f in "$prefix"/lib/libcwipc_*.so* "$prefix"/bin/cwipc_*; do
        [ -f "$f" ] && [ ! -L "$f" ] && file "$f" | grep -q ELF || continue
        check "$f" "$(ldd "$f" 2>&1)"
    done
    ;;
MINGW*|MSYS*|CYGWIN*)
    vswhere="/c/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
    vs=$("$vswhere" -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath | tr -d '\r')
    dumpbin=$(ls "$(cygpath -u "$vs")"/VC/Tools/MSVC/*/bin/Hostx64/x64/dumpbin.exe | tail -1)
    for f in "$prefix"/bin/cwipc_*.dll "$prefix"/bin/cwipc_*.exe; do
        [ -f "$f" ] || continue
        check "$f" "$("$dumpbin" -nologo -dependents "$(cygpath -w "$f")" | tr -d '\r')"
    done
    check "$prefix/bin (installed DLLs)" "$(ls "$prefix"/bin/*.dll 2>/dev/null)"
    ;;
*)
    echo "$0: unsupported platform $(uname -s)" >&2
    exit 2
    ;;
esac

if [ $found -ne 0 ]; then
    echo "$0: VTK, libpcl_io or Qt found in $prefix (cwi-dis/cwipc#357)"
    exit 1
fi
echo "$0: no VTK, libpcl_io or Qt in $prefix"
