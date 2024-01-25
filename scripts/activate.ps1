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
$candidateBinDir = $buildDir + "\bin\RelWithDebInfo"
if (Test-Path -Path $candidateBinDir) {
    $binDir = $candidateBinDir
} else {
    $binDir = $buildDir + "\bin\Release"
}
Write-Output $binDir
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
