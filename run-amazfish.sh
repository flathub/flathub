#!/bin/bash

cd /app/bin
./harbour-amazfishd &
daemon_pid=$!

function cleanup() {
    if [[ -n "$daemon_pid" && -e "/proc/$daemon_pid/exe" ]]; then
        echo "Terminating daemon $daemon_pid"
        kill "$daemon_pid"
    fi
}
trap cleanup EXIT

./harbour-amazfish-ui
