#!/bin/bash

exec flatpak-builder -y --force-clean --install-deps-from=flathub --install --user build io.liri.Calculator.yaml
