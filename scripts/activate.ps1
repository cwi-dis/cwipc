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
    if (-not (Get-Command bash -ErrorAction SilentlyContinue)) {
        Write-Error "activate.ps1: bash (Git Bash) is needed to run scripts/setup-cwipc.sh"
        return
    }
    & bash (Join-Path $PSScriptRoot "setup-cwipc.sh")
    if ($LASTEXITCODE -ne 0) {
        Write-Error "activate.ps1: scripts/setup-cwipc.sh failed"
        return
    }
}
. $generated
