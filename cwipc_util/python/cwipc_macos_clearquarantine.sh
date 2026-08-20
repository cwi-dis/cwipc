#!/bin/sh
# Find Actual INSTALL_PREFIX
bindir=`dirname $0`
bindir=`cd $bindir ; pwd`
installdir=`cd $bindir/.. ; pwd`
libdir="$installdir/lib"
set -x
xattr -r -d com.apple.quarantine $bindir/cwipc_*
xattr -r -d com.apple.quarantine  $libdir/libcwipc_*.dylib
