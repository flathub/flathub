#!/bin/bash

cd /app/bin
./harbour-amazfishd &
daemon_pid=$!
./harbour-amazfish-ui

kill "$daemon_pid"