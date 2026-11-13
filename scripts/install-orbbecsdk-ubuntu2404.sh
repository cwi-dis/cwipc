#!/bin/bash
# Versions: see versions.env (cwi-dis/cwipc#355).
source "$(dirname "$0")/versions.env"
ORBBECSDK="${ORBBECSDK_ARCHIVE_LINUX}"
CWIPCPATH="$( cd -- "$(dirname "$0")"/.. >/dev/null 2>&1 ; pwd -P )"
mkdir -p ${CWIPCPATH}/3rdparty_installed
cd ${CWIPCPATH}/3rdparty_installed
curl --location --output ${ORBBECSDK}.zip https://github.com/orbbec/OrbbecSDK_v2/releases/download/v${ORBBECSDK_VERSION}/${ORBBECSDK}.zip
rm -rf ${ORBBECSDK}
unzip ${ORBBECSDK}.zip
rm -f orbbecsdk
ln -s ${ORBBECSDK} orbbecsdk