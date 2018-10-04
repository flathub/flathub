#!/bin/bash

exec flatpak-builder -y --force-clean --install-deps-from=flathub --user --install build io.liri.BaseApp.yaml
