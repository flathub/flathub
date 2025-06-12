#!/bin/sh

# Find the path to the git repo
REPO=$(pwd)
if [ $# = 3 ] && [ "$1" -eq '-g' ] ; then
	REPO="$2"
	COMMIT="$3"
else
	COMMIT="$1"
fi

flatpak run --filesystem="$REPO" com.redhat.patchpal.gui -g "$REPO" "$COMMIT"
