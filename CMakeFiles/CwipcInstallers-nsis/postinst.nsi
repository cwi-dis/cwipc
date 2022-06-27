RequestExecutionLevel admin
ExecWait "powershell -ExecutionPolicy Bypass -WindowStyle Hidden -File $INSTDIR\share\cwipc\scripts\install-3rdparty-full-win1064.ps1"
ExecWait "$INSTDIR\bin\cwipc_pymodules_install.bat"
