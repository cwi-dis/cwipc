#!/bin/bash
# Get the current commit SHA
CURRENT_COMMIT=$(git rev-parse --short HEAD)
# Get the most recent tagged commit SHA
TAG_COMMIT=$(git rev-list --abbrev-commit --tags --max-count=1)
# Get the tag of that commit
TAG=$(git describe --abbrev=0 --tags $TAG_COMMIT)
# Remove a leading v and trailing _stable
TAG=${TAG/#v/}
TAG=${TAG/%_stable/}
# Construct version and version components
VERSION_COMMIT=""
VERSION_MAJOR=0
VERSION_MINOR=0
VERSION_MICRO=0
# Start with constructing the version
if [[ "$TAG" == "" ]] ; then
	# No tag found, use the current commit only
	VERSION="0.0.0+${CURRENT_COMMIT}"
	VERSION_COMMIT="+${CURRENT_COMMIT}"
else
	if [[ "$TAG_COMMIT" == "CURRENT_COMMIT" ]] ; then
		# Tag found, and we are exactly there
		VERSION="${TAG}"
	else
		VERSION="${TAG}+${CURRENT_COMMIT}"
		VERSION_COMMIT="+${CURRENT_COMMIT}"
	fi
fi
if [[ $VERSION =~ ^([0-9]+)\.([0-9]+)\.([0-9]+)\. ]]; then
	VERSION_MAJOR=${BASH_REMATCH[1]}
	VERSION_MINOR=${BASH_REMATCH[2]}
	VERSION_MICRO=${BASH_REMATCH[3]}
else
	if [[ $VERSION =~ ^([0-9]+)\.([0-9]+)\. ]]; then
		VERSION_MAJOR=${BASH_REMATCH[1]}
		VERSION_MINOR=${BASH_REMATCH[2]}
	fi
fi
echo VERSION=${VERSION}
echo VERSION_MAJOR=${VERSION_MAJOR}
echo VERSION_MINOR=${VERSION_MINOR}
echo VERSION_MICRO=${VERSION_MICRO}
echo VERSION_COMMIT=${VERSION_COMMIT}
