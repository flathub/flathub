#!/bin/sh

# create persistent config directory
mkdir -p ~/.nzbget

# create config from defaults if not exist
NZBGET_CONF=~/.nzbget/nzbget.conf
if [ ! -f "$NZBGET_CONF" ]; then
    cp /app/bin/webui/nzbget.conf.template "$NZBGET_CONF"
fi

if [ -n "$DISPLAY" ] || [ -n "$WAYLAND_DISPLAY" ]; then
    # running from a graphical environment
    # run nzbget as a daemon (single-instanced) and open webui
    NZBGET_PORT=$(cat "$NZBGET_CONF" | grep ControlPort | cut -d = -f 2)
    if [ -z "$NZBGET_PORT" ]; then
        NZBGET_PORT=6789
    fi
    if ! curl --silent --output /dev/null "http://127.0.0.1:$NZBGET_PORT"; then
        nzbget -D -c ~/.nzbget/nzbget.conf
    fi
    xdg-open "http://127.0.0.1:$NZBGET_PORT"
else
    # run nzbget as terminal application
    nzbget -s -c ~/.nzbget/nzbget.conf
fi
