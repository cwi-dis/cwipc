#!/bin/bash
#
# test-deb-package.sh DEB - install the cwipc Debian package, check it, remove it, and
# check that nothing of the setup is left (cwi-dis/cwipc#355). Needs sudo. Used by the
# Ubuntu CI build job; on a machine of your own only where installing cwipc is fine.
#
set -euo pipefail
deb="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"

echo "=== install $deb"
sudo apt-get install -y "$deb"

echo "=== what the postinst created"
test -x /usr/libexec/cwipc/venv/bin/python || { echo "FAIL: no venv in /usr/libexec/cwipc/venv"; exit 1; }
cat /usr/libexec/cwipc/launchers
ls -l /usr/bin/cwipc* | grep -- "->" || { echo "FAIL: no launcher symlinks in /usr/bin"; exit 1; }

echo "=== cwipc_check"
cwipc_check
echo "=== smoke test, without activating anything"
cwipc view --synthetic --count 3 --nodisplay

echo "=== remove"
sudo apt-get remove -y cwipc
if [ -e /usr/libexec/cwipc/venv ]; then echo "FAIL: venv left behind"; exit 1; fi
left=$(find /usr/bin -maxdepth 1 -name 'cwipc*' -type l)
if [ -n "$left" ]; then echo "FAIL: launcher symlinks left behind: $left"; exit 1; fi
echo "=== OK: the package installs, works and removes cleanly"
