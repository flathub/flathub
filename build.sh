#!/bin/bash

exec flatpak-builder -y --force-clean --install-deps-from=flathub --user build io.liri.Text.yaml
