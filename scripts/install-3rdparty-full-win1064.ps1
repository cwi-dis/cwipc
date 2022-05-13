# Install required third party packages.
# NEEDS ADMIN RIGHTS
# xxxjack Need to add entries to PATH environment
#
mkdir tmpinstall
cd tmpinstall

# Install libjpeg-turbo
(New-Object System.Net.WebClient).DownloadFile("https://sourceforge.net/projects/libjpeg-turbo/files/2.1.3/libjpeg-turbo-2.1.3-vc64.exe","libjpeg-turbo-2.1.3-vc64.exe");
Start-Process -FilePath '.\libjpeg-turbo-2.1.3-vc64.exe' -ArgumentList "/S" -Wait
# xxxjack add to path: C:\libjpeg-turbo64\bin

# Install PCL 1.11. At the moment 1.12 has issues on some CI/CD machines.
(New-Object System.Net.WebClient).DownloadFile("https://github.com/PointCloudLibrary/pcl/releases/download/pcl-1.11.1/PCL-1.11.1-AllInOne-msvc2019-win64.exe","PCL-1.11.1-AllInOne-msvc2019-win64.exe");
Start-Process -FilePath .\PCL-1.11.1-AllInOne-msvc2019-win64.exe -ArgumentList "/S" -Wait
# xxxjack add to path: C:\Program Files\PCL 1.11.1\bin
# xxxjack add to path: C:\Program Files\PCL 1.11.1\3rdParty\VTK\bin
# xxxjack add to path: C:\Program Files\OpenNI2\Redist

# Install Realsense SDK. Cannot get the installer to run unattended, so use chocolatey.
choco install -y realsense-sdk2
# xxxjack add to path: C:\Program Files (x86)\Intel RealSense SDK 2.0\bin\x64

# Install Kinect SDK
(New-Object System.Net.WebClient).DownloadFile("https://download.microsoft.com/download/3/d/6/3d6d9e99-a251-4cf3-8c6a-8e108e960b4b/Azure%20Kinect%20SDK%201.4.1.exe","Azure Kinect SDK 1.4.1.exe");
Start-Process -FilePath '.\Azure Kinect SDK 1.4.1.exe' -ArgumentList "/S" -Wait
# xxxjack add to path: C:\Program Files\Azure Kinect SDK v1.4.1\sdk\windows-desktop\amd64\release\bin

# Install Kinect Body Tracking SDK
(New-Object System.Net.WebClient).DownloadFile("https://download.microsoft.com/download/9/d/b/9dbe0fbe-c9c3-4228-a64c-1e0a08736ec1/Azure%20Kinect%20Body%20Tracking%20SDK%201.1.1.msi","Azure Kinect Body Tracking SDK 1.1.1.msi");
Start-Process '.\Azure Kinect Body Tracking SDK 1.1.1.msi' -ArgumentList '/quiet /passive' -Wait
# xxxjack add to path: C:\Program Files\Azure Kinect Body Tracking SDK\tools

# Install OpenCV
(New-Object System.Net.WebClient).DownloadFile("https://github.com/opencv/opencv/releases/download/4.5.5/opencv-4.5.5-vc14_vc15.exe","opencv-4.5.5-vc14_vc15.exe");
Start-Process '.\opencv-4.5.5-vc14_vc15.exe' -ArgumentList '-o"C:\" -y' -Wait
# xxxjack add to path: C:\opencv\build\bin
# xxxjack add to path: C:\opencv\build\x64\vc15\bin

# ADD TO PATH:
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

Add-PathVariable("C:\libjpeg-turbo64\bin")
Add-PathVariable("C:\Program Files\PCL 1.11.1\bin")
Add-PathVariable("C:\Program Files\PCL 1.11.1\3rdParty\VTK\bin")
Add-PathVariable("C:\Program Files\OpenNI2\Redist")
Add-PathVariable("C:\Program Files (x86)\Intel RealSense SDK 2.0\bin\x64")
Add-PathVariable("C:\Program Files\Azure Kinect SDK v1.4.1\sdk\windows-desktop\amd64\release\bin")
Add-PathVariable("C:\Program Files\Azure Kinect Body Tracking SDK\tools")
Add-PathVariable("C:\opencv\build\bin")
Add-PathVariable("C:\opencv\build\x64\vc15\bin")

Set-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name PATH -Value $env:Path
