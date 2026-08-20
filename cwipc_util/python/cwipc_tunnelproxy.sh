#!/bin/sh
localport=$1
remotehost=$2
remoteport=$3
case x$3 in
x)
	echo Usage $0 localport remotehost remoteport
	echo Forward cwipc_proxy connections incoming on localport to remotehost:remoteport
	exit 1
	;;
esac
innercmd="netcat --close $remotehost $remoteport"
trap exit SIGINT
set -x
while true; do
	netcat --verbose --listen --local-port=$localport --exec="$innercmd"
done
