#! /bin/env bash
export FDO_VERSION=24.08

# handle flathub repo configuration
# Most distros don't ship with this OOTB, and flathub opinionates to system scope config, but we're going to register this remote as 'flathub' under user so we have a reliable reference.
# This is recommended here: https://docs.flathub.org/docs/for-app-authors/submission/#build-and-install
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

#
# 32-Bit driver dependency resolution (local dev and test)
# check for nvidia kernel module, which is required, then install specific GL32 drivers that match
# This whole section is just to resolve the flathub/flatpak ecosystem problem here: https://github.com/flatpak/flatpak/issues/3072
#
# Install universal i386 (32-bit) compat extension for freedesktop
flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.Compat.i386//$FDO_VERSION

if [[ $(modinfo -F version nvidia 2>/dev/null) ]]; then
  echo 'Installing GL32 drivers matching live running nvidia kernel module'
  flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.GL32.nvidia-$(modinfo -F version nvidia | tr . -)//1.4
  echo 'Installing GL32 drivers matching flatpak-discovered Nvidia version'
else
  echo 'No kernel-module based nvidia driver detected by modinfo. Installing Mesa based GL32-default drivers instead to support intel/amd devices.' #We else off the modinfo command because flatpak drivers won't work if there's no running loaded kernel module to begin with'
  flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.GL32.default//$FDO_VERSION
  flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.GL32.default//$FDO_VERSION-extra # Leaving this here in case need to go back to fdo 23.08
fi
# In the future, Intel i915 drivers might also need to be checked and added.

#
# Build
#
if [[ ! $(flatpak list --app --user | grep org.flatpak.Builder) ]]; then
  echo 'Installing flatpak-builder via flatpak'
  flatpak install -u --noninteractive -y org.flatpak.Builder
fi

flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir com.projectrk.launcher.yml
