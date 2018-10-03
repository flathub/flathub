#!/bin/bash

exec flatpak-builder -y --force-clean --install-deps-from=flathub --user build io.liri.Terminal.yaml
