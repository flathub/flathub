#!/bin/env bash

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.projectrk.launcher.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
