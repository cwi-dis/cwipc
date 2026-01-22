#!/bin/bash
ORBBECSDK="OrbbecSDK_v2.6.3_202512231427_4e448f9_linux_x86_64"
CWIPCPATH="$( cd -- "$(dirname "$0")"/.. >/dev/null 2>&1 ; pwd -P )"
mkdir -p ${CWIPCPATH}/3rdparty_installed
cd ${CWIPCPATH}/3rdparty_installed
curl --location --output ${ORBBECSDK}.zip https://github.com/orbbec/OrbbecSDK_v2/releases/download/v2.6.3/${ORBBECSDK}.zip
rm -rf ${ORBBECSDK}
unzip ${ORBBECSDK}.zip
rm -f orbbecsdk
ln -s ${ORBBECSDK} orbbecsdk