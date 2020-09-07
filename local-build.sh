#!/usr/bin/env bash

BRANCH=test

mkdir _build
mkdir _repo

flatpak-builder --ccache --force-clean --default-branch=$BRANCH _build org.gnome.SoundJuicer.json --repo=_repo
flatpak build-bundle _repo org.gnome.SoundJuicer.flatpak org.gnome.SoundJuicer $BRANCH
