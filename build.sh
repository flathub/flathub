#! /bin/env bash

flatpak run org.flatpak.Builder --force-clean --user --install-deps-from=flathub --repo=repo --install context com.projectrk.launcher.yml
