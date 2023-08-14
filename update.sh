#!/usr/bin/env bash

flatpak run \
  --filesystem=$(pwd) \
  org.flathub.flatpak-external-data-checker \
  com.adilhanney.ricochlime.json \
  --update
