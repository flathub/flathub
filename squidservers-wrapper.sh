#!/usr/bin/env bash
set -e

# Run SquidServers using the Electron BaseApp zypak sandbox wrapper
exec zypak-wrapper.sh /app/extra/squidservers "$@"
