#Requires -RunAsAdministrator
#
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
# Function to test whether an executable can be run from PATH
#
Function Can-Execute-From-Path {
	param (
		[string]$command
	)
	try {
		Invoke-Expression -Command $command >$null 2>$null
	} catch {
		return 0
	}
	return 1
}

#
# Function to test whether a DLL can be loaded from PATH
#
# C# code (I think) to enable calling some Windows DLL functions
$registerDLL = @"
namespace RegisterDLL {
using System;
using System.Runtime.InteropServices;
    public static class Kernel32 {
        [DllImport("kernel32", SetLastError=true, CharSet = CharSet.Ansi)]
        public static extern IntPtr LoadLibrary([MarshalAs(UnmanagedType.LPStr)]string lpFileName); 
        [DllImport("kernel32", CharSet=CharSet.Ansi, ExactSpelling=true, SetLastError=true)]
        public static extern IntPtr GetProcAddress(IntPtr hModule, string procName);
        [DllImport("kernel32.dll", SetLastError=true)]
        [return: MarshalAs(UnmanagedType.Bool)]
        public static extern bool FreeLibrary(IntPtr hModule);
    }
    
    public static class User32 {
        //[DllImport("user32.dll")]
        //public static extern IntPtr CallWindowProc(WndProcDelegate lpPrevWndFunc, IntPtr hWnd, uint Msg, IntPtr wParam, IntPtr lParam);
        [DllImport("user32.dll")]
        public static extern IntPtr CallWindowProc(IntPtr wndProc, IntPtr hWnd, int msg, IntPtr wParam, IntPtr lParam);
    }
}
"@

Add-Type -TypeDefinition $registerDLL 

Function Is-DLL-On-Path {
	param (
		[string]$dllName
	)
	$module = [RegisterDLL.Kernel32]::LoadLibrary($dllName)
	$rv = $module -ne 0
	return $rv
}

#
# Create a temporary dirctory to download installers
#
if (Test-Path $env:TEMP\cwipc-3rdparty-downloads) {
	# Already exists
} else {
	mkdir $env:TEMP\cwipc-3rdparty-downloads
}
$tmpinstalldir="$((Get-Item $env:TEMP\cwipc-3rdparty-downloads).FullName)"

#
# Install libjpeg-turbo
#
if(Can-Execute-From-Path("jpegtran -help")) {
	Write-Output "libjpeg-turbo: already installed"
} else {
	Write-Output "libjpeg-turbo: downloading..."
	$installer="$tmpinstalldir\libjpeg-turbo-2.1.3-vc64.exe"
	(New-Object System.Net.WebClient).DownloadFile("https://sourceforge.net/projects/libjpeg-turbo/files/2.1.3/libjpeg-turbo-2.1.3-vc64.exe",$installer);
	Write-Output "libjpeg-turbo: installing..."
	Start-Process -FilePath $installer -ArgumentList "/S" -Wait
	Add-PathVariable("C:\libjpeg-turbo64\bin")
	Write-Output "libjpeg-turbo: installed"
}

#
# Install PCL 1.11. At the moment 1.12 has issues on some CI/CD machines.
#
$ok = Can-Execute-From-Path("pcl_generate -help")
if($ok) {
	Write-Output "pcl: already installed"
} else {
	Write-Output "pcl: downloading..."
	$installer="$tmpinstalldir\PCL-1.13.0-AllInOne-msvc2022-win64.exe"
	(New-Object System.Net.WebClient).DownloadFile("https://github.com/PointCloudLibrary/pcl/releases/download/pcl-1.13.0/PCL-1.13.0-AllInOne-msvc2022-win64.exe",$installer);
	Write-Output "pcl: installing..."
	Start-Process -FilePath $installer -ArgumentList "/S" -Wait
	Add-PathVariable("C:\Program Files\PCL 1.13.0\bin")
	Add-PathVariable("C:\Program Files\PCL 1.13.0\3rdParty\VTK\bin")
	Add-PathVariable("C:\Program Files\OpenNI2\Redist")
	Write-Output "pcl: installed"
}

#
# Install Realsense SDK. 
#
$ok = Is-DLL-On-Path("realsense2.dll")
if($ok) {
	Write-Output "intel-realsense: already installed"
} else {
	Write-Output "intel-realsense: downloading..."
	$installer="$tmpinstalldir\Intel.RealSense.SDK-WIN10-2.53.1.4623.exe"
	(New-Object System.Net.WebClient).DownloadFile(
	"https://github.com/IntelRealSense/librealsense/releases/download/v2.53.1/Intel.RealSense.SDK-WIN10-2.53.1.4623.exe",$installer);
	Write-Output "intel-realsense: installing..."
	Start-Process -FilePath $installer -ArgumentList '/VERYSILENT /SUPPRESSMSGBOXES /NORESTART /NOCANCEL /SP-' -Wait
	Add-PathVariable("C:\Program Files (x86)\Intel RealSense SDK 2.0\bin\x64")
	Write-Output "intel-realsense: installed"
}

#
# Install Kinect SDK
#
$ok = Is-DLL-On-Path("k4a.dll")
if($ok) {
	Write-Output "k4a: already installed"
} else {
	Write-Output "k4a: downloading..."
	$installer="$tmpinstalldir\Azure Kinect SDK 1.4.1.exe"
	(New-Object System.Net.WebClient).DownloadFile("https://download.microsoft.com/download/3/d/6/3d6d9e99-a251-4cf3-8c6a-8e108e960b4b/Azure%20Kinect%20SDK%201.4.1.exe", $installer);
	Write-Output "k4a: installing..."
	Start-Process -FilePath $installer -ArgumentList "/S" -Wait
	Add-PathVariable("C:\Program Files\Azure Kinect SDK v1.4.1\sdk\windows-desktop\amd64\release\bin")
	Write-Output "k4a: installed"
}

#
# Install Kinect Body Tracking SDK
#
$ok = Is-DLL-On-Path("k4abt.dll")
if($ok) {
	Write-Output "k4a-bt: already installed"
} else {
	Write-Output "k4a-bt: downloading..."
	$installer="$tmpinstalldir\Azure Kinect Body Tracking SDK 1.1.1.msi"
	(New-Object System.Net.WebClient).DownloadFile("https://download.microsoft.com/download/9/d/b/9dbe0fbe-c9c3-4228-a64c-1e0a08736ec1/Azure%20Kinect%20Body%20Tracking%20SDK%201.1.1.msi",$installer);
	Write-Output "k4a-bt: installing..."
	Start-Process $installer -ArgumentList '/quiet /passive' -Wait
	Add-PathVariable("C:\Program Files\Azure Kinect Body Tracking SDK\tools")
	Write-Output "k4a-bt: installed"
}
#
# Install OpenCV
#
$ok = Can-Execute-From-Path("opencv_version")
if($ok) {
	Write-Output "opencv: already installed"
} else {
	Write-Output "opencv: downloading..."
	$installer="$tmpinstalldir\opencv-4.5.5-vc14_vc15.exe"
	(New-Object System.Net.WebClient).DownloadFile("https://github.com/opencv/opencv/releases/download/4.5.5/opencv-4.5.5-vc14_vc15.exe",$installer);
	Write-Output "opencv: installing..."
	Start-Process $installer -ArgumentList '-o"C:\" -y' -Wait
	Add-PathVariable("C:\opencv\build\bin")
	Add-PathVariable("C:\opencv\build\x64\vc15\bin")
	Write-Output "opencv: installed"
}

#
# Finally save modified PATH environment variable to the registry.
#
Write-Output "registry: update PATH environment variable"
Set-ItemProperty -Path 'Registry::HKEY_LOCAL_MACHINE\System\CurrentControlSet\Control\Session Manager\Environment' -Name PATH -Value $env:Path
Write-Output "registry: done"
#
# Install Python. 
# NOTE: must be done last, due to manipulating PATH.
# NOTE: use pip, not python, because windows has an alias "python" that tells you how to install it.
#
$ok = Can-Execute-From-Path("pip --version")
if($ok) {
	Write-Output "python: already installed"
} else {
	Write-Output "python: downloading..."
	$installer="$tmpinstalldir\python-3.10.10-amd64.exe"
	(New-Object System.Net.WebClient).DownloadFile("https://www.python.org/ftp/python/3.10.10/python-3.10.10-amd64.exe",$installer);
	Write-Output "python: installing..."
	Start-Process -FilePath $installer -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1' -Wait
	Write-Output "python: installed"
}
