#!/bin/bash
case "$FLATPAK_ARCH" in
  "x86_64")
    export ELECTRON_FORGE_ARCH_ARGS='--arch=x64'
    ;;
  "aarch64")
    export ELECTRON_FORGE_ARCH_ARGS='--arch=arm64'
    ;;
esac

