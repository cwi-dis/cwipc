#!/bin/bash
if [[ "$1" != "-f" && $(git diff --stat) != "" ]]; then
	echo Not checking out, repo is dirty:
	git status
	git submodule foreach git status
	echo Error: Not checking out, repo is dirty. Use -f to override.
	exit
fi
if [[ "$1" == "-f" ]]; then
	shift
fi
branch=master
if [[ "$1" != "" ]]; then
	branch=$1
	shift
fi
set -x
git fetch --recurse-submodules
git checkout $branch
git pull --recurse-submodules
git submodule foreach git checkout $branch
git submodule foreach git pull
git status
