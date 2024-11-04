#! /bin/env bash

if [[ ! (flatpak list --app -u | grep org.flatpak.Builder)  ]]; then
  flatpak install -u --noninteractive -y org.flatpak.Builder
fi

flatpak run org.flatpak.Builder --force-clean --user --install-deps-from=flathub --repo=repo --install context com.projectrk.launcher.yml
