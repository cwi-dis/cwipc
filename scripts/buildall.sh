#!/bin/bash
set -x
set -e
dirname=`dirname $0`
dirname=`cd $dirname/..; pwd`
cd $dirname

cmakeargs=""

notest=
case x$1 in
x--notest)
	notest="notest"
	shift
	;;
esac

noinstall=
case x$1 in
x--noinstall)
	noinstall="noinstall"
	;;
esac

sudo=
case x$1 in
x--sudo)
	sudo="sudo"
	shift
	;;
esac

case x$1 in
x--cicd)
	cmakeargs="$cmakeargs -DCMAKE_INSTALL_PREFIX=$dirname/installed"
	shift
	;;
esac

# See if we can parallelize the build
if sysctl -n hw.physicalcpu 2>&1 >/dev/null; then
	ncpu=`sysctl -n hw.physicalcpu`
	export CMAKE_BUILD_PARALLEL_LEVEL=$ncpu
	export CTEST_PARALLEL_LEVEL=$ncpu
else
	if nproc 2>&1 >/dev/null; then
		ncpu=`nproc`
		export CMAKE_BUILD_PARALLEL_LEVEL=$ncpu
		export CTEST_PARALLEL_LEVEL=$ncpu
	fi
fi
set -x
# rm -rf build
cmake -S. -Bbuild $cmakeargs

cmake --build build
if [ "$notest" != "notest" ]; then
	ctest --test-dir build
fi

if [ "$noinstall" != "noinstall" ]; then
	$sudo cmake --install build
	
fi