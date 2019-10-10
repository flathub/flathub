#!/bin/bash

. /app/bin/envvars.sh

# start the server
cd /app/www/
python3 redirect_server.py
