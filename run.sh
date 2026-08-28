#!/usr/bin/env bash
set -euo pipefail

APP_DATA_DIR="${XDG_DATA_HOME:-$HOME/.var/app/io.github.theoninesixy.HMCL/data}"
HMCL_DIR="$APP_DATA_DIR"
WORK_DIR="$APP_DATA_DIR"
EXEC_PATH="/app/share/hmcl/HMCL.jar"

mkdir -p "$HMCL_DIR"
cd "$WORK_DIR"

exec java -Dhmcl.dir="$HMCL_DIR" -Dhmcl.home="$HMCL_DIR" -jar "$EXEC_PATH" "$@"