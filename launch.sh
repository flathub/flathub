#!/bin/sh
set -eu

APP_HOME="${XDG_DATA_HOME}"
mkdir -p "$APP_HOME"

cd "/app/share/gtask-for-desktop"
exec /bin/sh "./start.sh" "-Duser.home=$APP_HOME" "$@"
