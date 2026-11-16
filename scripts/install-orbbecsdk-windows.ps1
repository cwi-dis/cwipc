#
# install-orbbecsdk-windows.ps1 - install the OrbbecSDK, and optionally its viewer, into
# <repo>\3rdparty_installed, for building cwipc_orbbec from source (Windows).
#
# Usage: install-orbbecsdk-windows.ps1 [-Viewer]
#   -Viewer    Also install OrbbecViewer (for development: seeing what a camera does
#              without cwipc).
#
# Result: 3rdparty_installed\orbbecsdk (where CMake looks for the SDK) and
# 3rdparty_installed\orbbecviewer. Each holds a .cwipc-archive file that records the
# archive unpacked there, so what is already there in the version from versions.env
# is skipped. Installs nothing system-wide.
# The counterpart of install-orbbecsdk.sh (macOS, Linux). Versions and archive names:
# versions.env (cwi-dis/cwipc#351). Called by install-deps-windows.ps1 -Build.
#
param(
    [switch]$Viewer
)
$ErrorActionPreference = "Stop"
$progname = "install-orbbecsdk-windows.ps1"

$versions = @{}
Get-Content (Join-Path $PSScriptRoot "versions.env") | Where-Object { $_ -match '^\s*[A-Z_]+=' } | ForEach-Object {
    $key, $value = $_ -split '=', 2
    $versions[$key.Trim()] = $value.Trim()
}
$urlBase = "https://github.com/orbbec/OrbbecSDK_v2/releases/download/v" + $versions["ORBBECSDK_VERSION"]
$dest = Join-Path (Split-Path -Parent $PSScriptRoot) "3rdparty_installed"
New-Item -ItemType Directory -Force -Path $dest | Out-Null

# Unpack $archive (a .zip) into 3rdparty_installed\$link.
function Install-Archive([string]$archive, [string]$link) {
    $target = Join-Path $dest $link
    $marker = Join-Path $target ".cwipc-archive"
    if ((Test-Path $marker) -and ((Get-Content $marker -Raw).Trim() -eq $archive)) {
        Write-Output "${progname}: $archive already installed"
        return
    }
    if (-not $archive.EndsWith(".zip")) { throw "unknown archive type: $archive" }
    $tmp = Join-Path $dest (".download." + [System.IO.Path]::GetRandomFileName())
    New-Item -ItemType Directory -Path $tmp | Out-Null
    try {
        $zip = Join-Path $tmp $archive
        Write-Output "${progname}: downloading $archive"
        (New-Object System.Net.WebClient).DownloadFile("$urlBase/$archive", $zip)
        $unpacked = Join-Path $tmp "unpacked"
        Expand-Archive -LiteralPath $zip -DestinationPath $unpacked
        # Most archives have everything under one directory; the Windows ones have it
        # at the top.
        $content = $unpacked
        $entries = @(Get-ChildItem -Force $unpacked)
        if ($entries.Count -eq 1 -and $entries[0].PSIsContainer) { $content = $entries[0].FullName }
        if (Test-Path $target) { Remove-Item $target -Recurse -Force }
        Move-Item $content $target
        Set-Content -Path $marker -Value $archive
        Write-Output "${progname}: installed $target ($archive)"
    } finally {
        if (Test-Path $tmp) { Remove-Item $tmp -Recurse -Force }
    }
}

try {
    Install-Archive $versions["ORBBECSDK_ARCHIVE_WINDOWS"] "orbbecsdk"
    if ($Viewer) {
        Install-Archive $versions["ORBBECVIEWER_ARCHIVE_WINDOWS"] "orbbecviewer"
    }
} catch {
    Write-Output "${progname}: error: $_"
    exit 1
}
exit 0
