#!/bin/bash

# Disable the in-app update checker as updates are managed by flatpak.
export TW_DISABLE_UPDATE_CHECKER=1

exec zypak-wrapper /app/02engine/02engine "$@"
