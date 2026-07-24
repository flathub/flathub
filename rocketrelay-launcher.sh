#!/bin/sh
# extra-data payloads are fetched by the Flatpak client at install time as a
# raw file under /app/extra — never unpacked, and never visible to
# build-commands (which run before that fetch happens). So unpacking has to
# happen here, at first launch, into the app's writable data dir.
set -e

DATA_DIR="${XDG_DATA_HOME:-$HOME/.local/share}/rocketrelay"
GAME_BIN="${DATA_DIR}/Rocket Relay.x86_64"

if [ ! -x "${GAME_BIN}" ]; then
    mkdir -p "${DATA_DIR}"
    tar -xzf /app/extra/RocketRelay-Linux.tar.gz -C "${DATA_DIR}"
    chmod +x "${GAME_BIN}"
fi

exec "${GAME_BIN}" "$@"
