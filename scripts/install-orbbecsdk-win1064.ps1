#!/bin/bash
$ORBBECSDK = "OrbbecSDK_v2.6.3_202512232226_4e448f9_win_x64"
$ZIPFILE = $ORBBECSDK + ".zip"
$URL = "https://github.com/orbbec/OrbbecSDK_v2/releases/download/v2.6.3/" + $ZIPFILE

$scriptdir = $PSScriptRoot
$CWIPCPATH = Split-Path -Parent $scriptdir
$THIRDPARTY = Join-Path $CWIPCPATH 3rdparty_installed
$ZIPPATH = Join-Path $THIRDPARTY $ZIPFILE
New-Item -ItemType Directory -Force -Path $THIRDPARTY
Push-Location $THIRDPARTY
Remove-Item orbbecsdk -Recurse -Force
(New-Object System.Net.WebClient).DownloadFile($URL, $ZIPPATH)
Expand-Archive -LiteralPath $ZIPPATH -DestinationPath orbbecsdk
Pop-Location