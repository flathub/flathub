#!/bin/bash

set -e

FIRST_RUN="${XDG_CONFIG_HOME}/flatpak-vscode-first-run"

[ ! -f ${FIRST_RUN} ] && export WARNING_FILE="/app/share/vscode/flatpak-warning.txt"
touch ${FIRST_RUN}

for D in /usr/lib/sdk/*; do
  [ -f "${D}/enable.sh" ] && . "${D}/enable.sh"
done

exec env PATH="${PATH}:${XDG_DATA_HOME}/node_modules/bin" \
  /app/extra/vscode/bin/code --extensions-dir=${XDG_DATA_HOME}/vscode/extensions "$@" ${WARNING_FILE}
