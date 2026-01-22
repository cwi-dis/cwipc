#!/bin/bash
$ORBBECSDK = "OrbbecSDK_v2.6.3_202512232226_4e448f9_win_x64"
$ZIPFILE = $ORBBECSDK + ".zip"
$URL = "https://github.com/orbbec/OrbbecSDK_v2/releases/download/v2.6.3/" + $ZIPFILE

$scriptdir = $PSScriptRoot
$CWIPCPATH = Split-Path -Parent $scriptdir
$THIRDPARTY = Join-Path $CWIPCPATH 3rdparty_installed
New-Item -ItemType Directory -Force -Path $THIRDPARTY
Set-Location $THIRDPARTY
(New-Object System.Net.WebClient).DownloadFile($URL, $ZIPFILE)
Remove-Item $ORBBECSDK -Force
Remove-Item orbbecsdk -Force
Expand-Archive -LiteralPath $zipfile -DestinationPath orbecsdk
