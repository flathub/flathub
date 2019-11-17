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

if [[ -d /usr/lib/sdk/node10 ]]; then
  . /usr/lib/sdk/node10/enable.sh
fi

if [[ -d /usr/lib/sdk/php73 ]]; then
  . /usr/lib/sdk/php73/enable.sh
fi

if [[ -d /usr/lib/sdk/openjdk8 ]]; then
  . /usr/lib/sdk/openjdk8/enable.sh
fi

if [[ -d /usr/lib/sdk/mono5 ]]; then
  . /usr/lib/sdk/mono5/enable.sh
fi

exec env PATH="${PATH}:${XDG_DATA_HOME}/node_modules/bin" \
  /app/extra/vscode/bin/code --extensions-dir=${XDG_DATA_HOME}/vscode/extensions "$@" ${WARNING_FILE}
