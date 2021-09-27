#!/bin/bash
set -x
set -e
dirname=`dirname $0`
dirname=`cd $dirname/..; pwd`
cd $dirname
notest=
case x$1 in
x--notest)
	notest="notest"
	shift
	;;
esac
sudo=
case x$1 in
x--sudo)
	sudo="sudo"
	shift
	;;
esac
cmakeargs=
case x$1 in
x--cicd)
	cmakeargs="$cmakeargs -DCMAKE_INSTALL_PREFIX=$dirname/installed"
	shift
	;;
esac
# Workaround for brew-installed Qt5 not found by cmake:
if [ -d /usr/local/opt/qt5/lib/cmake/Qt5 ]; then
	cmakeargs="$cmakeargs -DQt5_DIR=/usr/local/opt/qt5/lib/cmake/Qt5"
fi
mkdir -p build
cd build
cmake .. $cmakeargs
make
if [ "$notest" != "notest" ]; then
	make test
fi
$sudo make install

