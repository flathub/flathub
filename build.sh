#!/bin/bash

flatpak-builder \
  --repo=repo \
  --disable-rofiles-fuse \
  --install-deps-from=flathub \
  --force-clean \
  --default-branch=master \
  --arch=x86_64 --ccache \
  build-dir/ dev.overlayed.Overlayed.yaml