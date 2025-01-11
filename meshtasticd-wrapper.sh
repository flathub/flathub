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
    cp /app/share/meshtasticd/config-dist.yaml $CONF_DIR/config.yaml
    sed -i 's|/usr/share/meshtasticd|/app/share/meshtasticd|g' $CONF_DIR/config.yaml
    sed -i "s|/etc/meshtasticd|$CONF_DIR|g" $CONF_DIR/config.yaml
fi

exec meshtasticd "--fsdir=$XDG_DATA_HOME" "--config=$XDG_CONFIG_HOME/meshtasticd/config.yaml" "$@"
