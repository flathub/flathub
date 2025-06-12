#!/usr/bin/env bash

BRANCH=test
ARCH=${ARCH:-}

if [ -n "$ARCH" ] ; then
	ARCH_OPT="--arch $ARCH"
	ARCH_SUFFIX="-$ARCH"
fi

rm -f com.redhat.cee.Patchpal$ARCH_SUFFIX.flatpak
rm -rf _build ; mkdir _build
rm -rf _repo ; mkdir _repo

flatpak-builder $ARCH_OPT --ccache --force-clean --default-branch=$BRANCH _build com.redhat.patchpal.gui.yaml --repo=_repo
flatpak build-bundle $ARCH_OPT _repo com.redhat.patchpal.gui$ARCH_SUFFIX.flatpak com.redhat.patchpal.gui $BRANCH
