#! /bin/env bash
export FDO_VERSION=24.08

# handle flathub repo configuration
# Most distros don't ship with this OOTB, and flathub opinionates to system scope config, but we're going to register this remote as 'flathub' under user so we have a reliable reference.
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo

# Install universal i386 (32-bit) compat extension for freedesktop
flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.Compat.i386//$FDO_VERSION

# check for nvidia kernel module, which is required, then install specific GL32 drivers that match
if [[ $(modinfo -F version nvidia 2>/dev/null) ]]; then
  echo 'Installing GL32 drivers matching live running nvidia kernel module'
  flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.GL32.nvidia-$(modinfo -F version nvidia | tr . -)//1.4
  echo 'Installing GL32 drivers matching flatpak-discovered Nvidia version'
else
  echo 'No kernel-module based nvidia driver detected by modinfo. Installing Mesa based GL32-default drivers instead to support intel/amd devices.' #We else off the modinfo command because flatpak drivers won't work if there's no running loaded kernel module to begin with'
  flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.GL32.default//$FDO_VERSION
  flatpak install -u -y --noninteractive flathub org.freedesktop.Platform.GL32.default//$FDO_VERSION-extra
fi

if [[ ! $(flatpak list --app --user | grep org.flatpak.Builder) ]]; then
  echo 'Installing flatpak-builder via flatpak'
  flatpak install -u --noninteractive -y org.flatpak.Builder
fi

flatpak run org.flatpak.Builder --force-clean --user --repo=repo --install --install-deps-from=flathub context com.projectrk.launcher.yml
