#
# install-deps-windows.ps1 - install or check cwipc's dependencies on Windows.
#
# Usage: install-deps-windows.ps1 [-Runtime | -Build]
#   -Runtime   What an installed cwipc tree needs to run (default): Python. The native
#              DLLs come with the installer or archive. Installs Python if missing
#              and running as administrator; otherwise reports what to do.
#   -Build     Also what is needed to build cwipc from source. Checks and reports
#              Visual Studio (C++), CMake, git, git-lfs and NSIS (installing those
#              needs admin rights and takes long, so this script doesn't), and does
#              what needs no admin rights: OrbbecSDK into <repo>\3rdparty_installed,
#              and bootstrapping vcpkg.
#
# Exit status: 0 if everything needed is there, 1 if something is missing.
# Python packages are not installed here: the setup script installs them into the venv.
# What goes in which level, and why: cwi-dis/cwipc#355 (decision of 2026-11-13).
# Versions: versions.env.
#
param(
    [switch]$Runtime,
    [switch]$Build
)
$ErrorActionPreference = "Stop"
$progname = "install-deps-windows.ps1"

$versions = @{}
Get-Content (Join-Path $PSScriptRoot "versions.env") | Where-Object { $_ -match '^\s*[A-Z_]+=' } | ForEach-Object {
    $key, $value = $_ -split '=', 2
    $versions[$key.Trim()] = $value.Trim()
}
$pythonVersion = $versions["PYTHON_VERSION"]
$pythonVersionMin = $versions["PYTHON_VERSION_MIN"]
$pythonInstallerVersion = $versions["PYTHON_WINDOWS_INSTALLER_VERSION"]

$missing = @()

function Test-Command([string]$name) {
    return [bool](Get-Command $name -ErrorAction SilentlyContinue)
}

function Test-IsAdmin {
    $identity = [Security.Principal.WindowsIdentity]::GetCurrent()
    return (New-Object Security.Principal.WindowsPrincipal $identity).IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

# Is a Python between $pythonVersionMin and $pythonVersion available? Use the py launcher
# if there is one ("python" may be the Microsoft Store alias that only tells you how to
# install Python).
function Test-Python {
    if (Test-Command "py") {
        foreach ($v in @($pythonVersion, $pythonVersionMin)) {
            & py "-$v" --version *> $null
            if ($LASTEXITCODE -eq 0) { return $true }
        }
    }
    if (Test-Command "python") {
        $v = (& python --version 2>&1) -join ""
        if ($v -match "^Python (\d+\.\d+)\.") {
            $found = [version]$Matches[1]
            if ($found -ge [version]$pythonVersionMin -and $found -le [version]$pythonVersion) { return $true }
        }
    }
    return $false
}

#
# Runtime level: Python.
#
if (Test-Python) {
    Write-Output "${progname}: Python ${pythonVersion}: found"
} elseif (Test-IsAdmin) {
    $installer = Join-Path $env:TEMP "python-$pythonInstallerVersion-amd64.exe"
    Write-Output "${progname}: Python ${pythonVersion}: downloading $pythonInstallerVersion"
    (New-Object System.Net.WebClient).DownloadFile("https://www.python.org/ftp/python/$pythonInstallerVersion/python-$pythonInstallerVersion-amd64.exe", $installer)
    Write-Output "${progname}: Python ${pythonVersion}: installing"
    Start-Process -FilePath $installer -ArgumentList '/quiet InstallAllUsers=1 PrependPath=1' -Wait
    $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User")
    if (-not (Test-Python)) { $missing += "Python $pythonVersion (installation failed)" }
} else {
    $missing += "Python $pythonVersion (run this script as administrator to install it, or: winget install Python.Python.$pythonVersion)"
}

#
# Build level: check and report, then OrbbecSDK and vcpkg.
#
if ($Build) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} "Microsoft Visual Studio\Installer\vswhere.exe"
    $vs = $null
    if (Test-Path $vswhere) {
        $vs = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    }
    if ($vs) {
        Write-Output "${progname}: Visual Studio with C++: found ($vs)"
    } else {
        $missing += "Visual Studio with the 'Desktop development with C++' workload (https://visualstudio.microsoft.com/)"
    }
    foreach ($tool in @(
        @{ Name = "CMake"; Command = "cmake"; Hint = "winget install Kitware.CMake" },
        @{ Name = "git"; Command = "git"; Hint = "winget install Git.Git" }
    )) {
        if (Test-Command $tool.Command) {
            Write-Output "${progname}: $($tool.Name): found"
        } else {
            $missing += "$($tool.Name) ($($tool.Hint))"
        }
    }
    if (Test-Command "git") {
        & git lfs version *> $null
        if ($LASTEXITCODE -eq 0) {
            Write-Output "${progname}: git-lfs: found"
        } else {
            $missing += "git-lfs (comes with Git for Windows; then: git lfs install)"
        }
    }
    if ((Test-Command "makensis") -or (Test-Path (Join-Path ${env:ProgramFiles(x86)} "NSIS\makensis.exe"))) {
        Write-Output "${progname}: NSIS: found"
    } else {
        Write-Output "${progname}: NSIS: not found (only needed to build the installer: winget install NSIS.NSIS)"
    }

    & (Join-Path $PSScriptRoot "install-orbbecsdk-windows.ps1")

    $repo = Split-Path -Parent $PSScriptRoot
    $bootstrap = Join-Path $repo "vcpkg\bootstrap-vcpkg.bat"
    if (Test-Path (Join-Path $repo "vcpkg\vcpkg.exe")) {
        Write-Output "${progname}: vcpkg: already bootstrapped"
    } elseif (Test-Path $bootstrap) {
        Write-Output "${progname}: vcpkg: bootstrapping"
        & $bootstrap -disableMetrics
    } else {
        $missing += "vcpkg submodule (git submodule update --init vcpkg)"
    }
}

if ($missing.Count -gt 0) {
    Write-Output "${progname}: missing:"
    $missing | ForEach-Object { Write-Output "  - $_" }
    exit 1
}
Write-Output "${progname}: all dependencies present"
exit 0
