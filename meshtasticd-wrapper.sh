#!/usr/bin/env bash

CONF_DIR="$XDG_CONFIG_HOME/meshtasticd"

# Ensure conf directories exist
mkdir -p "$CONF_DIR/config.d"
mkdir -p "$CONF_DIR/available.d"

# Always copy available.d config files
cp -R /app/share/meshtasticd/available.d/* "$CONF_DIR/available.d/"

# Copy default config if it doesn't exist
if [ ! -f "$CONF_DIR/config.yaml" ]; then
    echo "Copying default config to $CONF_DIR/config.yaml"
    cp /app/share/meshtasticd/config-dist.yaml "$CONF_DIR/config.yaml"
    sed -i 's|/usr/share/meshtasticd|/app/share/meshtasticd|g' "$CONF_DIR/config.yaml"
    sed -i "s|/etc/meshtasticd|$CONF_DIR|g" "$CONF_DIR/config.yaml"
fi

# Copy default UI config if it doesn't exist
if [ ! -f "$CONF_DIR/config.d/MUI.yaml" ]; then
    echo "Copying default UI config to $CONF_DIR/config.d/MUI.yaml"
    cp /app/share/meshtasticd/available.d/MUI/X11_480x480.yaml "$CONF_DIR/config.d/MUI.yaml"
fi

# Link default map tiles if they don't exist
if [ ! -d "$XDG_DATA_HOME/map" ]; then
    echo "Linking default map tiles to /app/share/meshtasticd/maps/positron"
    ln -s /app/share/meshtasticd/maps/positron "$XDG_DATA_HOME/map"
fi

exec meshtasticd "--fsdir=$XDG_DATA_HOME" "--config=$XDG_CONFIG_HOME/meshtasticd/config.yaml" "$@"
