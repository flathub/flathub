#!/bin/bash

# When run from the vscode dev container this will build a .flatpak of the app.

set -e

set -x


projectId=codes.merritt.adventurelist


# git submodule update --remote --merge --init --recursive
flatpak-builder --force-clean --disable-cache --disable-rofiles-fuse build-dir $projectId.yml --repo=repo
flatpak build-bundle repo $projectId.flatpak $projectId
