#!/bin/bash
if [[ "$1" != "-f" && $(git diff --stat) != "" ]]; then
	echo Repo is dirty:
	git diff
	exit
fi
set -x
git fetch --recurse-submodules
git checkout master
git pull --recurse-submodules
git submodule foreach git checkout master
git submodule foreach git pull
git status
