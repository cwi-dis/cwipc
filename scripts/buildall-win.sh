#!/bin/bash
set -e
set -x
errorexit() {
	echo '** Error: your buildall-win did not succeed. Check error log above.'
	exit 1
}
# Alternative: config=RelWithDebInfo.
# Alternative: config=Release. But having the .PDB files is good for debugging.
config=Release
trap errorexit ERR
dirname=`dirname $0`
dirname=`cd $dirname/..; pwd`
cd $dirname
instdir=c:/cwipc
case x$1 in
x--vrtogether)
	instdir=../installed
	shift
	;;
x--vrtdebug)
	instdir=../installed
	config=RelWithDebInfo
	shift
	;;
esac
mkdir -p $instdir
instdir=`cd $instdir; pwd`
instdir=`cygpath -w "$instdir"`

notest=
case x$1 in
x--notest)
	notest="notest"
	shift
	;;
esac

if nproc 2>&1 >/dev/null; then
	ncpu=`nproc`
	makeargs="$makeargs -j $ncpu"
	export CTEST_PARALLEL_LEVEL=$ncpu
	export CMAKE_BUILD_PARALLEL_LEVEL=$ncpu
fi

mkdir -p build
cmake -S . -B build -DCMAKE_INSTALL_PREFIX="$instdir"
cmake --build build --config $config
if [ "$notest" != "notest" ]; then
	ctest --test-dir build --build-config $config
fi
cmake --install build --config $config
