#!/bin/bash

set -e

FIRST_RUN="${XDG_CONFIG_HOME}/flatpak-vscode-first-run"

function msg() {
  echo "flatpak-vscode: $*" >&2
}

if [ ! -f ${FIRST_RUN} ]; then
  WARNING_FILE="/app/share/vscode-insiders/flatpak-warning.txt"
  touch ${FIRST_RUN}
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

exec env PATH="${PATH}:${XDG_DATA_HOME}/node_modules/bin" \
  /app/extra/vscode-insiders/bin/code-insiders --extensions-dir=${XDG_DATA_HOME}/vscode-insiders/extensions "$@" ${WARNING_FILE}
