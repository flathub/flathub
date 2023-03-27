#!/bin/sh

PORT=8096

if ! curl --silent http://localhost:$PORT > /dev/null; then
  flatpak run org.jellyfin.Jellyfin & sleep 10
fi

xdg-open http://localhost:$PORT
