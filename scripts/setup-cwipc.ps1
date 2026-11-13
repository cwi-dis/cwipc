#
# setup-cwipc.ps1 - set up an installed cwipc tree on Windows: optionally install the
# dependencies (Python), create a Python venv with the cwipc modules, write the per-tree
# files, and optionally put the Python launchers in bin.
#
# The PowerShell counterpart of setup-cwipc.sh for installed trees (the Windows installer
# and "cwipc_check install", which run on machines without bash). Source trees and CI use
# setup-cwipc.sh through Git Bash. Design: cwi-dis/cwipc#354 and #355.
#
# Usage: setup-cwipc.ps1 [-Prefix DIR] [-Deps] [-NoPython] [-Launchers] [-Python PY] [-PauseOnError]
#   -Prefix DIR     The tree. Default: found from this script's location
#                   (<prefix>\libexec\cwipc\scripts\setup-cwipc.ps1).
#   -Deps           Also install the runtime dependencies (install-deps-windows.ps1 -Runtime).
#   -NoPython       Don't create the venv or install the Python modules.
#   -Launchers      Copy the Python launchers (cwipc.exe, cwipc_view.exe, ...) into
#                   <prefix>\bin, listed in <prefix>\libexec\cwipc\launchers.
#   -Python PY      Python to create the venv with. Default: "py -<PYTHON_VERSION>" if the
#                   py launcher has it, else "python".
#   -PauseOnError   Wait for Enter before closing if something failed (used when this
#                   runs in its own elevated window).
#
# Writes in <prefix>\libexec\cwipc\: activate.ps1 (run it to get PATH,
# CWIPC_LIBRARY_DIR and the venv), activate (the same for Git Bash) and environment
# (machine-readable, see setup-cwipc.sh).
#
param(
    [string]$Prefix = "",
    [switch]$Deps,
    [switch]$NoPython,
    [switch]$Launchers,
    [string]$Python = "",
    [switch]$PauseOnError
)
$ErrorActionPreference = "Stop"
$progname = "setup-cwipc.ps1"

function Fail([string]$message) {
    Write-Output "${progname}: $message"
    if ($PauseOnError) { Read-Host "Press Enter to close this window" | Out-Null }
    exit 1
}

try {
    $versions = @{}
    Get-Content (Join-Path $PSScriptRoot "versions.env") | Where-Object { $_ -match '^\s*[A-Z_]+=' } | ForEach-Object {
        $key, $value = $_ -split '=', 2
        $versions[$key.Trim()] = $value.Trim()
    }

    #
    # Find the prefix.
    #
    if ($Prefix -eq "") {
        $scriptsDir = $PSScriptRoot
        if ((Split-Path -Leaf $scriptsDir) -ne "scripts" -or (Split-Path -Leaf (Split-Path -Parent $scriptsDir)) -ne "cwipc") {
            Fail "cannot determine the cwipc tree from the script location $scriptsDir (use -Prefix; for a source tree use setup-cwipc.sh)"
        }
        $Prefix = Split-Path -Parent (Split-Path -Parent (Split-Path -Parent $scriptsDir))
    }
    $Prefix = (Resolve-Path $Prefix).Path
    $binDir = Join-Path $Prefix "bin"
    if (-not (Test-Path $binDir)) { Fail "$Prefix does not look like a cwipc tree (no bin)" }
    $libexec = Join-Path $Prefix "libexec\cwipc"
    $venv = Join-Path $libexec "venv"
    $venvScripts = Join-Path $venv "Scripts"
    $venvPython = Join-Path $venvScripts "python.exe"
    $wheelDir = Join-Path $Prefix "share\cwipc\python"
    New-Item -ItemType Directory -Force -Path $libexec | Out-Null

    #
    # Dependencies.
    #
    if ($Deps) {
        Write-Output "${progname}: installing dependencies (runtime level)"
        & (Join-Path $PSScriptRoot "install-deps-windows.ps1") -Runtime
        if ($LASTEXITCODE -ne 0) { Fail "install-deps-windows.ps1 failed" }
        # A Python installed just now is on the machine's PATH, not yet on ours.
        $env:Path = [System.Environment]::GetEnvironmentVariable("Path", "Machine") + ";" + [System.Environment]::GetEnvironmentVariable("Path", "User") + ";" + $env:Path
    }

    #
    # Python venv with the cwipc modules.
    #
    if (-not $NoPython) {
        $pythonCmd = @()
        if ($Python -ne "") {
            $pythonCmd = @($Python)
        } else {
            $pyVersion = $versions["PYTHON_VERSION"]
            $havePy = $false
            if (Get-Command py -ErrorAction SilentlyContinue) {
                & py "-$pyVersion" --version *> $null
                $havePy = ($LASTEXITCODE -eq 0)
            }
            if ($havePy) { $pythonCmd = @("py", "-$pyVersion") } else { $pythonCmd = @("python") }
        }
        $pyExe = $pythonCmd[0]
        $pyArgs = @($pythonCmd | Select-Object -Skip 1)
        if (-not (Get-Command $pyExe -ErrorAction SilentlyContinue)) { Fail "Python $($versions['PYTHON_VERSION']) not found (use -Deps to install it, or -Python)" }
        # cwipc is built and tested with exactly this version (pinned, cwi-dis/cwipc#355).
        $pyFound = (& $pyExe @pyArgs -c "import sys; print('%d.%d' % sys.version_info[:2])" 2>$null) -join ""
        if ($pyFound -ne $versions["PYTHON_VERSION"]) { Fail "Python $($versions['PYTHON_VERSION']) needed, but $($pythonCmd -join ' ') is $pyFound (use -Deps to install it, or -Python)" }
        Write-Output "${progname}: creating venv $venv with $(& $pyExe @pyArgs --version 2>&1)"
        & $pyExe @pyArgs -m venv --clear $venv
        if ($LASTEXITCODE -ne 0) { Fail "creating the venv failed" }
        & $venvPython -m pip install --quiet --upgrade pip
        if ($LASTEXITCODE -ne 0) { Fail "upgrading pip failed" }
        $wheels = @(Get-ChildItem -Path $wheelDir -Filter "cwipc_*.whl" | ForEach-Object { $_.FullName })
        if ($wheels.Count -eq 0) { Fail "no cwipc wheels in $wheelDir" }
        Write-Output "${progname}: installing the cwipc Python modules from $wheelDir"
        & $venvPython -m pip install --quiet --upgrade --find-links=$wheelDir @wheels
        if ($LASTEXITCODE -ne 0) { Fail "installing the cwipc Python modules failed" }
    }

    #
    # Per-tree files: environment, activate.ps1, activate (Git Bash).
    #
    $envLines = @("# Generated by $progname. Format: see the comment at the top of setup-cwipc.sh.", "path $binDir")
    if (-not $NoPython) { $envLines += @("path $venvScripts", "env VIRTUAL_ENV=$venv") }
    $envLines += "env CWIPC_LIBRARY_DIR=$binDir"
    Set-Content -Path (Join-Path $libexec "environment") -Value $envLines

    $ps1Lines = @("# Generated by ${progname}: run in PowerShell to set up the environment.",
        "`$Env:Path = `"$binDir;`" + `$Env:Path",
        "`$Env:CWIPC_LIBRARY_DIR = `"$binDir`"")
    if (-not $NoPython) { $ps1Lines += "& `"$(Join-Path $venvScripts 'Activate.ps1')`"" }
    Set-Content -Path (Join-Path $libexec "activate.ps1") -Value $ps1Lines

    $unixBin = ($binDir -replace '\\', '/') -replace '^([A-Za-z]):', '/$1'
    $shLines = @("# Generated by ${progname}: use with `"source`" in Git Bash, don't execute.",
        "export PATH=`"${unixBin}:`$PATH`"",
        "export CWIPC_LIBRARY_DIR=`"$binDir`"")
    if (-not $NoPython) { $shLines += "source `"$(($venvScripts -replace '\\', '/') -replace '^([A-Za-z]):', '/$1')/activate`"" }
    Set-Content -Path (Join-Path $libexec "activate") -Value $shLines

    # Before cwi-dis/cwipc#355 the activate files were at the top of the prefix.
    foreach ($f in @((Join-Path $Prefix "activate"), (Join-Path $Prefix "activate.ps1"))) {
        if ((Test-Path $f) -and ((Get-Content $f -TotalCount 1) -match '^# Generated by setup-cwipc')) { Remove-Item $f }
    }

    #
    # Launchers in bin.
    #
    if ($Launchers -and -not $NoPython) {
        $launcherList = Join-Path $libexec "launchers"
        if (Test-Path $launcherList) {
            Get-Content $launcherList | Where-Object { $_ -ne "" } | ForEach-Object {
                $old = Join-Path $binDir $_
                if (Test-Path $old) { Remove-Item $old -Force }
            }
        }
        $placed = @()
        Get-ChildItem -Path $venvScripts -Filter "cwipc*.exe" | ForEach-Object {
            $target = Join-Path $binDir $_.Name
            if (Test-Path $target) {
                Write-Output "${progname}: warning: not replacing $target with the launcher from the venv"
            } else {
                Copy-Item -Path $_.FullName -Destination $target
                $placed += $_.Name
            }
        }
        Set-Content -Path $launcherList -Value $placed
        Write-Output "${progname}: launchers in ${binDir}: $($placed -join ' ')"
    }

    Write-Output "${progname}: done. Use: & `"$(Join-Path $libexec 'activate.ps1')`""
    exit 0
} catch {
    Fail "error: $_"
}
