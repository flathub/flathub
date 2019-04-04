#!/bin/bash

set -e

FIRST_RUN="${XDG_CONFIG_HOME}/flatpak-vscode-first-run"

if [ ! -f ${FIRST_RUN} ]; then
  WARNING_FILE="/app/share/vscode/flatpak-warning.txt"
  touch ${FIRST_RUN}
fi

if [[ -d /usr/lib/sdk/dotnet ]]; then
  . /usr/lib/sdk/dotnet/enable.sh
fi

exec env PATH="${PATH}:${XDG_DATA_HOME}/node_modules/bin" \
  /app/extra/vscode/bin/code --extensions-dir=${XDG_DATA_HOME}/vscode/extensions "$@" ${WARNING_FILE}
