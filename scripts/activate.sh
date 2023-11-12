# Set PATH and DYLD_LIBRARY_PATH to include built executables and dynamic libraries and enable Python venv.
# use with "source", don't execute.
myPath=`realpath $0`
myDir=`dirname $myPath`
buildDir=`cd $myDir/../build && pwd`
export PATH="$buildDir/bin:$PATH"
# Setting DYLD_LIBRARY_PATH doesn't work, but luckily it usually isn't needed
export DYLD_LIBRARY_PATH="$buildDir/lib:$DYLD_LIBRARY_PATH"
export CWIPC_LIBRARY_DIR="$buildDir/lib"
source $buildDir/venv/bin/activate
# Install editable Python packages
(cd $buildDir/../cwipc_util/python && python -m pip install -e .)
(cd $buildDir/../cwipc_codec/python && python -m pip install -e .)
(cd $buildDir/../cwipc_realsense2/python && python -m pip install -e .)
(cd $buildDir/../cwipc_kinect/python && python -m pip install -e .)
