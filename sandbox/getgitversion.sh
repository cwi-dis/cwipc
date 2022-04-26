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
if [[ "$TAG" == "" ]] ; then
	# No tag found, use the current commit only
	VERSION="0.0.0-${CURRENT_COMMIT}"
else
	if [[ "$TAG_COMMIT" == "CURRENT_COMMIT" ]] ; then
		# Tag found, and we are exactly there
		VERSION="${TAG}"
	else
		VERSION="${TAG}+${CURRENT_COMMIT}"
	fi
fi
echo VERSION=${VERSION}