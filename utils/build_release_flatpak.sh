#!/bin/bash
flatpak-builder --force-clean --install --delete-build-dirs \
  _build org.chromium.Chromium.yaml
