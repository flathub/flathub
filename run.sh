#!/bin/bash

if [ ! $WAYLAND_DISPLAY ]; then 
	exec zypak-wrapper /app/bin/ledger-live-desktop --ozone-platform=x11 "$@"
else
	exec zypak-wrapper /app/bin/ledger-live-desktop "$@"
fi
