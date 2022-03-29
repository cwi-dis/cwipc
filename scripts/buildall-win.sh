#!/bin/bash
set -e
set -x
errorexit() {
	echo '** Error: your buildall-win did not succeed. Check error log above.'
	exit 1
}
# Alternative: config=Release. But having the .PDB files is good for debugging.
config=RelWithDebInfo
trap errorexit ERR
dirname=`dirname $0`
dirname=`cd $dirname/..; pwd`
cd $dirname
instdir=installed
case x$1 in
x--vrtogether)
	instdir=../installed
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
cd build
cmake .. -G "Visual Studio 16 2019" -DCMAKE_INSTALL_PREFIX="$instdir" -DJPEG_Turbo_ROOT="C:/libjpeg-turbo64" -DOpenCV_DIR="C:/OpenCV-4.5.5/build"
cmake --build . --config $config
if [ "$notest" != "notest" ]; then
	cmake --build . --config $config --target RUN_TESTS
fi
cmake --build . --config $config --target INSTALL

