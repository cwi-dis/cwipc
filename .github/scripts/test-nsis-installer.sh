#!/bin/bash
#
# test-nsis-installer.sh INSTALLER - install the cwipc Windows installer silently,
# check it (including the Start-menu .bat files), uninstall it, and check that the
# installation directory is gone (cwi-dis/cwipc#355). Git Bash, as administrator. Used by
# the test-installer-windows-intel CI job, on a fresh runner.
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

# The installer puts bin on the machine's PATH, but this shell started before that: the
# situation of a terminal that was open during the installation. cwipc_check must say
# that the installation works, but not from this environment (exit status 1).
echo "=== cwipc_check, without bin on PATH: must diagnose the environment"
status=0
"$destu/bin/cwipc_check.exe" > cwipc_check.out 2>&1 || status=$?
cat cwipc_check.out
if [ "$status" -eq 0 ]; then
    echo "FAIL: cwipc_check succeeded without bin on PATH"; exit 1
fi
grep -q "The installation itself works, but not from this environment" cwipc_check.out || { echo "FAIL: cwipc_check did not diagnose the environment"; exit 1; }
rm cwipc_check.out

# From here on, as in a new terminal. The Start-menu shortcuts run .bat files that expect
# bin on PATH. They end with "pause".
export PATH="$destu/bin:$PATH"
echo "=== cwipc_check, with bin on PATH: must succeed"
"$destu/bin/cwipc_check.exe"
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
