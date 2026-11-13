#!/bin/bash
#
# test-nsis-installer.sh INSTALLER - install the cwipc Windows installer silently,
# check it (including the Start-menu .bat files), uninstall it, and check that the
# installation directory is gone (cwi-dis/cwipc#355). Git Bash, as administrator. Used by
# the Windows CI build job.
#
set -euo pipefail
installer="$(cygpath -w "$1")"
dest='C:\cwipc-installer-test'
destu=$(cygpath -u "$dest")

echo "=== install $installer into $dest"
# /D must be the last argument and unquoted. Start-Process -Wait: the installer may
# return before it's done otherwise.
powershell -NoProfile -Command "\$p = Start-Process -FilePath '$installer' -ArgumentList '/S','/D=$dest' -Wait -PassThru; exit \$p.ExitCode"

echo "=== what cwipc_check install (run by the installer) created"
test -f "$destu/libexec/cwipc/venv/Scripts/python.exe" || { echo "FAIL: no venv"; exit 1; }
test -f "$destu/bin/cwipc.exe" || { echo "FAIL: no cwipc.exe launcher in bin"; exit 1; }
cat "$destu/libexec/cwipc/launchers"

echo "=== cwipc_check"
"$destu/bin/cwipc_check.exe"

# The Start-menu shortcuts run .bat files that expect bin on PATH (the installer puts it
# on the machine's PATH; this shell started before that). They end with "pause".
export PATH="$destu/bin:$PATH"
cd "$destu/libexec/cwipc/scripts"
echo "=== Start menu: Check cwipc installation"
echo | cmd //c run-cwipc-check.bat
echo "=== Start menu: Attempt to fix cwipc installation"
echo | cmd //c run-cwipc-check-fix.bat
# run-cwipc-view-synthetic.bat opens a viewer without a frame limit; run its command
# with limits instead.
echo "=== Start menu (equivalent): View a sample dynamic pointcloud"
cwipc_view --synthetic --count 3 --nodisplay
cd /

echo "=== uninstall"
"$destu/Uninstall.exe" //S
# The uninstaller copies itself elsewhere and returns at once: wait for it.
for i in $(seq 1 120); do
    [ -d "$destu" ] || break
    sleep 1
done
if [ -d "$destu" ]; then
    echo "FAIL: $dest left behind after uninstall:"
    find "$destu" | head -50
    exit 1
fi
echo "=== OK: the installer installs, works and uninstalls cleanly"
