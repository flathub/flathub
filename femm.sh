#!/bin/bash

export WINEPREFIX="${HOME}/.local/share/femm42-flatpak"
export WINEDLLOVERRIDES="mscoree,mshtml="

POE_INSTALLER_NAME="femm42bin_x64_21Apr2019.exe"
POE_SETUP="${WINEPREFIX}/${POE_INSTALLER_NAME}"
POE_DOWNLOAD_URL='https://www.femm.info/wiki/Files/files.xml?action=download&file=femm42bin_x64_21Apr2019.exe'
POE_RUN_CMD="${WINEPREFIX}/drive_c/femm42/bin/femm.exe"

WINE="/app/bin/wine"

XORG_LOG="/var/log/Xorg.0.log"

VERSION_NUM="1.0.0"
VERSION_FILE="${WINEPREFIX}/info.femm.unofficial"

declare -ra WINE_PACKAGES=(directx9 corefonts tahoma win7)
declare -ra WINE_SETTINGS=('csmt=on' 'glsl=disabled')

echo "#############################################"
echo "## FEMM 4.2 Unofficial Flatpak v${VERSION_NUM} ##"
echo "#############################################"
echo

set_wine_settings(){
  local my_documents="${WINEPREFIX}/drive_c/users/${USER}/My Documents"

  echo "Installing wine requirements."
  winetricks --unattended "${WINE_PACKAGES[@]}"

  echo "Setting wine settings."
  winetricks --unattended "${WINE_SETTINGS[@]}"

  echo
}

# Run only if POE isn't installed
first_run(){
  set_wine_settings

  echo "${VERSION_NUM}" > "${VERSION_FILE}"

  if [ ! -f "${POE_SETUP}" ]; then
    echo "Downloading FEMM 4.2."
    curl -o "${POE_SETUP}" -O -J -L "${POE_DOWNLOAD_URL}" 
  fi
  
  # Installl Compat.i386 if not found
  flatpak-spawn --host flatpak list|grep org.freedesktop.Platform.Compat.i386|grep 21.08 || \
  flatpak-spawn --host flatpak install -y org.freedesktop.Platform.Compat.i386/x86_64/21.08

  echo "Running FEMM 4.2."
  "${WINE}" "${POE_SETUP}"
}

is_updated(){
  if [ -f "${VERSION_FILE}" ]; then
    last_version="$(cat ${VERSION_FILE})"
  else
    last_version="0"
  fi

  echo "${VERSION_NUM}" > "${VERSION_FILE}"
  
  if [[ "${VERSION_NUM}" == "${last_version}" ]]; then
    return 0
  else
    return 1
  fi
}

# Main function
startup(){
  if ! grep 'Software\\\\Gedanken Magnetics\\\\FEMM\\\\4\\\\femm' "${WINEPREFIX}/user.reg" >/dev/null; then
    echo "FEMM not installed."
    first_run
  else
    if ! is_updated; then
      echo "Not up to date, re-run wine settings"
      set_wine_settings
    fi
  fi
  
  if ! grep 'flatpak' "${WINEPREFIX}/drive_c/femm42/mfiles/openfemm.m" >/dev/null; then
    echo "Adjusting Octave and Python compatibility"
    cp -f /app/mfiles/* ${WINEPREFIX}/drive_c/femm42/mfiles
    cp -f /app/wine-bash ${WINEPREFIX}/wine
  fi

  echo ; echo "Starting FEMM 4.2..."
  "${WINE}" "${POE_RUN_CMD}"
}

startup
