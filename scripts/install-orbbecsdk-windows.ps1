# Install OrbbecSDK into 3rdparty_installed/orbbecsdk (PowerShell).
# Versions: see versions.env (cwi-dis/cwipc#355).
$versions = @{}
Get-Content (Join-Path $PSScriptRoot "versions.env") | Where-Object { $_ -match '^\s*[A-Z_]+=' } | ForEach-Object {
    $key, $value = $_ -split '=', 2
    $versions[$key.Trim()] = $value.Trim()
}
$ORBBECSDK = $versions["ORBBECSDK_ARCHIVE_WINDOWS"]
$ZIPFILE = $ORBBECSDK + ".zip"
$URL = "https://github.com/orbbec/OrbbecSDK_v2/releases/download/v" + $versions["ORBBECSDK_VERSION"] + "/" + $ZIPFILE

$scriptdir = $PSScriptRoot
$CWIPCPATH = Split-Path -Parent $scriptdir
$THIRDPARTY = Join-Path $CWIPCPATH 3rdparty_installed
$ZIPPATH = Join-Path $THIRDPARTY $ZIPFILE
New-Item -ItemType Directory -Force -Path $THIRDPARTY | Out-Null
Push-Location $THIRDPARTY
# orbbecsdk\.cwipc-archive records which archive was unpacked there.
$MARKER = Join-Path orbbecsdk ".cwipc-archive"
if ((Test-Path $MARKER) -and ((Get-Content $MARKER -Raw).Trim() -eq $ORBBECSDK)) {
    Write-Output "install-orbbecsdk-windows.ps1: $ORBBECSDK already installed"
    Pop-Location
    exit 0
}
if (Test-Path orbbecsdk) {
    Remove-Item orbbecsdk -Recurse -Force
}
(New-Object System.Net.WebClient).DownloadFile($URL, $ZIPPATH)
Expand-Archive -LiteralPath $ZIPPATH -DestinationPath orbbecsdk
Set-Content -Path $MARKER -Value $ORBBECSDK
Pop-Location