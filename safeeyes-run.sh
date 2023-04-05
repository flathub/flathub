#!/bin/bash
if [[ -f "${XDG_CONFIG_HOME}/safeeyes/safeeyes.json" ]]; then
    safeeyes "$@"
else
    echo "Created safeeyes.json"
    mkdir -p "${XDG_CONFIG_HOME}/safeeyes/style" && touch "${XDG_CONFIG_HOME}/safeeyes/safeeyes.json"
    safeeyes "$@"
fi