#!/bin/bash

set -e
shopt -s nullglob

FIRST_RUN="${XDG_CONFIG_HOME}/flatpak-devin-first-run"
VSCODE_CONFIG_DIR="${XDG_CONFIG_HOME}/Code/User"
VSCODE_SETTINGS_FILE="${VSCODE_CONFIG_DIR}/settings.json"

function msg() {
  echo "flatpak-devin: $*" >&2
}

if [ ! -f ${FIRST_RUN} ]; then
  WARNING_FILE="/app/share/devin/flatpak-warning.txt"
  touch ${FIRST_RUN}
  
  # Copy VS Code terminal configuration on first run
  if [ ! -f ${VSCODE_SETTINGS_FILE} ]; then
    mkdir -p "${VSCODE_CONFIG_DIR}"
    cp /app/share/devin/vscode-settings.json "${VSCODE_SETTINGS_FILE}"
    msg "VS Code terminal configuration installed"
  fi
fi

if [ "$FLATPAK_ENABLE_SDK_EXT" = "*" ]; then
  SDK=()
  for d in /usr/lib/sdk/*; do
    SDK+=("${d##*/}")
  done
else
  IFS=',' read -ra SDK <<< "$FLATPAK_ENABLE_SDK_EXT"
fi

for i in "${SDK[@]}"; do
  if [[ -d /usr/lib/sdk/$i ]]; then
    msg "Enabling SDK extension \"$i\""
    if [[ -f /usr/lib/sdk/$i/enable.sh ]]; then
      . /usr/lib/sdk/$i/enable.sh
    else
      export PATH=$PATH:/usr/lib/sdk/$i/bin
    fi
  else
    msg "Requested SDK extension \"$i\" is not installed"
  fi
done

if [ ! -e /etc/shells ] && [ -e /var/run/host/etc/shells ]; then
  ln -s /var/run/host/etc/shells /etc/shells
fi

exec env ELECTRON_RUN_AS_NODE=1 PATH="${PATH}:${XDG_DATA_HOME}/node_modules/bin" \
  /app/bin/zypak-wrapper.sh /app/extra/devin/devin-desktop /app/extra/devin/resources/app/out/cli.js \
  --extensions-dir="${XDG_DATA_HOME}/devin/extensions" \
  "$@" ${WARNING_FILE}
