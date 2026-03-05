#Requires -RunAsAdministrator
#
# Install required third party packages.
# NEEDS ADMIN RIGHTS
#
# First check whether we are running under gitbuh actions
#
$global:ghActionRunner = Test-Path env:\GITHUB_ACTIONS


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
# Create a temporary dirctory to download installers
#
if (Test-Path $env:TEMP\cwipc-3rdparty-downloads) {
	# Already exists
} else {
	mkdir $env:TEMP\cwipc-3rdparty-downloads
}
$tmpinstalldir="$((Get-Item $env:TEMP\cwipc-3rdparty-downloads).FullName)"


#
# Install Python. 
#
# NOTE: use pip, not python, because windows has an alias "python" that tells you how to install it.
#
$ok = Can-Execute-From-Path("pip --version")
if($ok) {
	Write-Output "cwipc_check install: python: already installed"
} else {
	Write-Output "cwipc_check install: python: downloading..."
	$installer="$tmpinstalldir\python-3.12.10-amd64.exe"
	(New-Object System.Net.WebClient).DownloadFile("https://www.python.org/ftp/python/3.12.10/python-3.12.10-amd64.exe",$installer);
	Write-Output "cwipc_check install: python: installing..."
	Start-Process -FilePath $installer -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1' -Wait
	Write-Output "cwipc_check install: python: installed"
}
#
# Install Python packages
#
if ($global:ghActionRunner) {
	Write-Output "cwipc_check install: python: cwipc packages: skipping."
} else {
	Write-Output "cwipc_check install: python: reload PATH..."
	$env:Path = [System.Environment]::GetEnvironmentVariable("Path","Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path","User") 
	Write-Output "cwipc_check install: creating venv..."
	$venvDir = "$PSScriptRoot\..\..\..\libexec\cwipc\venv"
	python -m venv --clear $venvDir
	Write-Output "cwipc_check install: activating venv..."
	& $venvDir\Scripts\Activate.ps1
	Write-Output "cwipc_check install: python: cwipc packages: installing..."
	$binDir = "$PSScriptRoot\..\..\..\bin"
	& $binDir\cwipc_pymodules_install.ps1
	Write-Output "cwipc_check install: Copy python scripts to bin directory..."
	Copy-Item -Path "$venvDir\Scripts\cwipc*.exe" -Destination "$binDir" -Force
	Write-Output "cwipc_check install: python: cwipc packages: All done."
}
Write-Output "cwipc_check install: All done: You can close this Powershell window."
