#!/usr/bin/env bash

set -ex

# Cheating the process that the executable files are in FAKE_BIN_DIR
FAKE_BIN_DIR="$XDG_CONFIG_HOME/bin"

export TODESK_PACK_NAME=todesk
export LIBVA_DRIVER_NAME=iHD
export LIBVA_DRIVERS_PATH="$APP_HOME/todesk/bin"

# Load the hack library.
export LD_PRELOAD=/app/lib/libtodesk_fix.so

TODESK_EXEC_PATH="$FAKE_BIN_DIR/ToDesk_Service" "/app/extra/todesk/bin/ToDesk_Service" &
TODESK_EXEC_PATH="$FAKE_BIN_DIR/ToDesk" exec "/app/extra/todesk/bin/ToDesk"
