#!/bin/sh

PORT=47990

if ! curl http://localhost:$PORT > /dev/null 2>&1; then
  (sleep 3 && xdg-open http://localhost:$PORT) &
  sunshine
else
  xdg-open https://localhost:$PORT
fi
