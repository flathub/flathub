#!/bin/sh

CONFIG_DIR="$HOME/.config/ScriptedJourneys"

if [ ! -d "$CONFIG_DIR" ]; then
    mkdir -p "$CONFIG_DIR"
    cp -r /app/lib/maps $CONFIG_DIR
    cp /app/lib/LICENSE $CONFIG_DIR
    mkdir -p $CONFIG_DIR/playerdata
    touch $CONFIG_DIR/playerdata/playerdata.json
    python3 /app/bin/setup.py
fi
