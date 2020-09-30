#!/bin/bash
flatpak-builder --force-clean --install --delete-build-dirs \
  _build com.google.Chromium.yaml
