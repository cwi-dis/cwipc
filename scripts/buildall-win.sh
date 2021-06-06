#!/bin/bash
set -e
set -x
errorexit() {
	echo '** Error: your buildall-win did not succeed. Check error log above.'
	exit 1
}
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
case x$1 in
x)
	all="cwipc_util cwipc_realsense2 cwipc_codec cwipc_kinect"
	;;
*)
	all=$@
	;;
esac

for i in $all ; do
(
	cd $i
	(
		mkdir -p build
		cd build
		cmake .. -G "Visual Studio 16 2019" -DCMAKE_INSTALL_PREFIX="$instdir" -DJPEG_Turbo_ROOT="C:/libjpeg-turbo64"
		cmake --build . --config Release
		if [ "$notest" != "notest" ]; then
			cmake --build . --config Release --target RUN_TESTS
		fi
		cmake --build . --config Release --target INSTALL
	)	
)
done
