# Install required third party packages.
# NEEDS ADMIN RIGHTS
# xxxjack Need to add entries to PATH environment
#
# Function to add entries to PATH environment variable
#
Function Add-PathVariable {
    param (
        [string]$addPath
    )
    if (Test-Path $addPath){
        $regexAddPath = [regex]::Escape($addPath)
        $arrPath = $env:Path -split ';' | Where-Object {$_ -notMatch 
"^$regexAddPath\\?"}
        $env:Path = ($arrPath + $addPath) -join ';'
    } else {
        Throw "'$addPath' is not a valid path."
    }
}

#
# Create a temporary dirctory to download installers
#
mkdir tmpinstall
$tmpinstalldir="$((Get-Item .\tmpinstall).FullName)"
#
# Install libjpeg-turbo
#
$installer="$tmpinstalldir\libjpeg-turbo-2.1.3-vc64.exe"
(New-Object System.Net.WebClient).DownloadFile("https://sourceforge.net/projects/libjpeg-turbo/files/2.1.3/libjpeg-turbo-2.1.3-vc64.exe",$installer);
Start-Process -FilePath $installer -ArgumentList "/S" -Wait
Add-PathVariable("C:\libjpeg-turbo64\bin")

#
# Install PCL 1.11. At the moment 1.12 has issues on some CI/CD machines.
#
$installer="$tmpinstalldir\PCL-1.11.1-AllInOne-msvc2019-win64.exe"
(New-Object System.Net.WebClient).DownloadFile("https://github.com/PointCloudLibrary/pcl/releases/download/pcl-1.11.1/PCL-1.11.1-AllInOne-msvc2019-win64.exe",$installer);
Start-Process -FilePath $installer -ArgumentList "/S" -Wait
Add-PathVariable("C:\Program Files\PCL 1.11.1\bin")
Add-PathVariable("C:\Program Files\PCL 1.11.1\3rdParty\VTK\bin")
Add-PathVariable("C:\Program Files\OpenNI2\Redist")

#
# Install Realsense SDK. Cannot get the installer to run unattended, so use chocolatey.
#
$installer="$tmpinstalldir\Intel.RealSense.SDK-WIN10-2.50.0.3785.exe"
(New-Object System.Net.WebClient).DownloadFile("https://github.com/IntelRealSense/librealsense/releases/download/v2.50.0/Intel.RealSense.SDK-WIN10-2.50.0.3785.exe",$installer);
Start-Process -FilePath $installer -ArgumentList '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /NOCANCEL /SP-' -Wait
Add-PathVariable("C:\Program Files (x86)\Intel RealSense SDK 2.0\bin\x64")

#
# Install Kinect SDK
#
$installer="$tmpinstalldir\Azure Kinect SDK 1.4.1.exe"
(New-Object System.Net.WebClient).DownloadFile("https://download.microsoft.com/download/3/d/6/3d6d9e99-a251-4cf3-8c6a-8e108e960b4b/Azure%20Kinect%20SDK%201.4.1.exe", $installer);
Start-Process -FilePath $installer -ArgumentList "/S" -Wait
Add-PathVariable("C:\Program Files\Azure Kinect SDK v1.4.1\sdk\windows-desktop\amd64\release\bin")

#
# Install Kinect Body Tracking SDK
#
$installer="$tmpinstalldir\Azure Kinect Body Tracking SDK 1.1.1.msi"
(New-Object System.Net.WebClient).DownloadFile("https://download.microsoft.com/download/9/d/b/9dbe0fbe-c9c3-4228-a64c-1e0a08736ec1/Azure%20Kinect%20Body%20Tracking%20SDK%201.1.1.msi",$installer);
Start-Process $installer -ArgumentList '/quiet /passive' -Wait
Add-PathVariable("C:\Program Files\Azure Kinect Body Tracking SDK\tools")

#
# Install OpenCV
#
$installer="$tmpinstalldir\opencv-4.5.5-vc14_vc15.exe"
(New-Object System.Net.WebClient).DownloadFile("https://github.com/opencv/opencv/releases/download/4.5.5/opencv-4.5.5-vc14_vc15.exe",$installer);
Start-Process $installer -ArgumentList '-o"C:\" -y' -Wait
Add-PathVariable("C:\opencv\build\bin")
Add-PathVariable("C:\opencv\build\x64\vc15\bin")

#
# Finally save modified PATH environment variable to the registry.
#
Set-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name PATH -Value $env:Path
