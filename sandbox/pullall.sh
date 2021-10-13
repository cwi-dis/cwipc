#!/bin/bash
if [[ $(git diff --stat) != "" ]]; then
	echo Repo is dirty
	exit
fi
set -x
git fetch --recurse-submodules
git checkout master
git pull --recurse-submodules
git submodule foreach git checkout master
git submodule foreach git pull
git status
