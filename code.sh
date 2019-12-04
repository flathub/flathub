#!/bin/bash

set -e

FIRST_RUN="${XDG_CONFIG_HOME}/flatpak-vscode-first-run"

if [ ! -f ${FIRST_RUN} ]; then
  WARNING_FILE="/app/share/vscode/flatpak-warning.txt"
  touch ${FIRST_RUN}
fi

IFS=',' read -ra SDK <<< "$FLATPAK_ENABLE_SDK"
for i in "${SDK[@]}"; do
  if [[ -d /usr/lib/sdk/$i ]]; then
    . /usr/lib/sdk/$i/enable.sh
  fi
done

exec env PATH="${PATH}:${XDG_DATA_HOME}/node_modules/bin" \
  /app/extra/vscode/bin/code --extensions-dir=${XDG_DATA_HOME}/vscode/extensions "$@" ${WARNING_FILE}
