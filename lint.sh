#!/bin/env bash

flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest com.project_rk.launcher.yml
flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream metadata/com.project_rk.launcher.metainfo.xml
