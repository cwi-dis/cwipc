# Set up the environment for using cwipc from this source tree (PowerShell, Windows).
# Run it with ". scripts\activate.ps1" (or "& scripts\activate.ps1").
#
# Transitional (cwi-dis/cwipc#355): the real work is done by setup-cwipc.sh (run with
# Git Bash), which writes build\libexec\cwipc\activate.ps1. This file runs it the first
# time, then runs the result.
# To redo the setup (for example after building another configuration):
#   bash scripts/setup-cwipc.sh [--config Release]
$topDir = Split-Path -Parent $PSScriptRoot
$generated = Join-Path $topDir "build\libexec\cwipc\activate.ps1"
if (-not (Test-Path $generated)) {
    # Git Bash: the bash.exe that comes with git.exe (Git\cmd\git.exe or
    # Git\mingw64\bin\git.exe -> Git\bin\bash.exe). Not just "bash" on PATH: that may be
    # WSL's (System32\bash.exe), which can't run this with a Windows path.
    $bash = $null
    $git = Get-Command git -ErrorAction SilentlyContinue
    if ($git) {
        $dir = Split-Path -Parent $git.Source
        foreach ($up in 1..3) {
            $dir = Split-Path -Parent $dir
            if ($dir -and (Test-Path (Join-Path $dir "bin\bash.exe"))) { $bash = Join-Path $dir "bin\bash.exe"; break }
        }
    }
    if (-not $bash) {
        $b = Get-Command bash -ErrorAction SilentlyContinue
        if ($b -and $b.Source -notmatch '\\System32\\') { $bash = $b.Source }
    }
    if (-not $bash) {
        Write-Error "activate.ps1: Git Bash is needed to run scripts/setup-cwipc.sh (it comes with Git for Windows)"
        return
    }
    # Forward slashes: bash takes backslashes as escape characters.
    & $bash ((Join-Path $PSScriptRoot "setup-cwipc.sh") -replace '\\', '/')
    if ($LASTEXITCODE -ne 0) {
        Write-Error "activate.ps1: scripts/setup-cwipc.sh failed"
        return
    }
}
. $generated
