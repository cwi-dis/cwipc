# Set PATH to include built executables and dynamic libraries and enable Python venv.
# For use in Powershell, for example from within VSCode.
# Set-PSDebug -Trace 2
$mypath = $MyInvocation.MyCommand.Path
Write-Output $myPath
$myDir = Split-Path $mypath -Parent
Write-Output $myDir
$topDir = Split-Path $myDir -Parent
Write-Output $topDir
$buildDir = $topDir + "\build"
Write-Output $buildDir
$candidate1BinDir = $buildDir + "\bin\Debug"
$candidate2BinDir = $buildDir + "\bin\RelWithDebInfo"
$candidate3BinDir = $buildDir + "\bin\Release"
if (Test-Path -Path $candidate1BinDir) {
    $binDir = $candidate1BinDir
} elseif (Test-Path -Path $candidate2BinDir) {
    $binDir = $candidate2BinDir
} elseif (Test-Path -Path $candidate3BinDir) {
    $binDir = $candidate3BinDir
} else {
    $binDir = $buildDir + "\bin"
}
Write-Output "binDir = " + $binDir
$binDirExists = Test-Path $binDir
if (-Not $binDirExists) {
    Write-Output "binDir does not exist. Not enabling venv"
    exit
}
$Env:CWIPC_LIBRARY_DIR = $binDir
$Env:PATH = $binDir + ";" + $Env:PATH
Write-Output $Env:PATH
& $buildDir/venv/Scripts/Activate.ps1
# Install editable Python packages
cd $topDir/cwipc_util/python
& python -m pip install -e .
cd $topDir/cwipc_codec/python
& python -m pip install -e .
cd $topDir/cwipc_realsense2/python
& python -m pip install -e .
cd $topDir/cwipc_kinect/python
& python -m pip install -e .
cd $topDir
