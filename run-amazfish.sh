#!/bin/bash

cd /app/bin
./harbour-amazfishd &
daemon_pid=$!
./harbour-amazfish-ui &
ui_pid=$!
wait "$ui_pid"

kill "$daemon_pid"