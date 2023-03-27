#!/bin/sh

PORT=8096

if ! curl http://localhost:$PORT > /dev/null 2>&1; then
  (sleep 10 && xdg-open http://localhost:$PORT) &
  jellyfin
else
  xdg-open http://localhost:$PORT
fi
