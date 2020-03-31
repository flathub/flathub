#!/bin/bash

set -e

FIRST_RUN="${XDG_CONFIG_HOME}/flatpak-vscodium-first-run"

if [ ! -f ${FIRST_RUN} ]; then
  WARNING_FILE="/app/share/codium/flatpak-warning.txt"
  touch ${FIRST_RUN}
fi

IFS=',' read -ra SDK <<< "$FLATPAK_ENABLE_SDK"
for i in "${SDK[@]}"; do
  if [[ -d /usr/lib/sdk/$i ]]; then
    if [[ -f /usr/lib/sdk/$i/enable.sh ]]; then
      . /usr/lib/sdk/$i/enable.sh
    else
      export PATH=$PATH:/usr/lib/sdk/$i/bin
    fi
  fi
done

exec env PATH="${PATH}:${XDG_DATA_HOME}/node_modules/bin" \
  /app/share/codium/bin/codium --extensions-dir=${XDG_DATA_HOME}/codium/extensions "$@" ${WARNING_FILE}
