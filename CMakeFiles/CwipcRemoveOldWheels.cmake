# Remove a package's wheels from earlier builds (cwi-dis/cwipc#355).
# Their version includes the commit hash, so without this they accumulate in the
# wheel directory, get installed together and make pip fail with conflicting versions.
# Usage: cmake -DWHEELDIR=<dir> -DNAME=<package> -P CwipcRemoveOldWheels.cmake
file(GLOB _old_wheels "${WHEELDIR}/${NAME}-*.whl")
if(_old_wheels)
	file(REMOVE ${_old_wheels})
endif()
