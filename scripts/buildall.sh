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
	cmakeargs="-DCMAKE_INSTALL_PREFIX=$dirname/installed"
	shift
	;;
esac
case x$1 in
x)
	all="cwipc_util cwipc_realsense2 cwipc_codec"
	if [ `uname` != "Darwin" ]; then
		all="$all cwipc_kinect"
	fi
	;;
*)
	all=$@
	;;
esac
	
for i in $all ; do
if [ -d $i ]; then
(
	echo "** Building $i **"
	cd $i
	(
		mkdir -p build
		cd build
		cmake .. $cmakeargs
		make
		if [ "$notest" != "notest" ]; then
			make test
		fi
		$sudo make install
	)	
)
else
	echo "** Skipping $i - not found **"
fi
done
