#!/bin/bash

. /app/bin/envvars.sh

run_kolibri.sh &

# open the browser once the redirect port is ready
false; while [ $? -ne 0 ]; do sleep 0.1; echo "import socket; import sys; sys.exit(socket.socket(socket.AF_INET, socket.SOCK_STREAM).connect_ex(('127.0.0.1', ${REDIRECT_PORT})))" | python3; done
xdg-open http://127.0.0.1:${REDIRECT_PORT}/
