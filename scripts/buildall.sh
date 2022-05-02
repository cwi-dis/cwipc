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

# See if we can parallelize the build
if sysctl -n hw.physicalcpu 2>&1 >/dev/null; then
	ncpu=`sysctl -n hw.physicalcpu`
	export CTEST_BUILD_PARALLEL_LEVEL=$ncpu
	export CTEST_PARALLEL_LEVEL=$ncpu
fi
if nproc 2>&1 >/dev/null; then
	ncpu=`nproc`
	export CTEST_BUILD_PARALLEL_LEVEL=$ncpu
	export CTEST_PARALLEL_LEVEL=$ncpu
fi
set -x
rm -rf build
cmake -S. -Bbuild $cmakeargs

cmake --build build
if [ "$notest" != "notest" ]; then
	(cd build && ctest )
fi

$sudo cmake --install build

