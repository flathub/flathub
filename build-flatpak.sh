#!/usr/bin/env bash

set -e

flatpak-builder --force-clean --user --install builddir me.acristoffers.void.yml
