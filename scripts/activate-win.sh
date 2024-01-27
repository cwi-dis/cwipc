# Set PATH and DYLD_LIBRARY_PATH to include built executables and dynamic libraries and enable Python venv.
# use with "source", don't execute.
if [[ "${BASH_SOURCE}" == "" ]]; then
    myPath=`realpath "${(%):-%N}"`
else
    myPath=`realpath "${BASH_SOURCE}"`
fi
myDir=`dirname $myPath`
buildDir=`cd $myDir/../build && pwd`
if [ -d $buildDir/RelWithDebInfo ]; then
	binDir="$buildDir/bin/RelWithDebInfo"
else
	binDir="$buildDir/bin/Release"
fi
export PATH="$binDir:$PATH"
export CWIPC_LIBRARY_DIR=`cygpath -w $binDir`
source $buildDir/venv/Scripts/activate
# Install editable Python packages
(cd $buildDir/../cwipc_util/python && python -m pip install -e .)
(cd $buildDir/../cwipc_codec/python && python -m pip install -e .)
(cd $buildDir/../cwipc_realsense2/python && python -m pip install -e .)
(cd $buildDir/../cwipc_kinect/python && python -m pip install -e .)

