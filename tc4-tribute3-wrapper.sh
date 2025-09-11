#!/bin/bash
# T-Crisis 4 Flatpak Wrapper Script
# Changes to the correct directory before running the game

set -e

cd /app/bin
exec ./tc4-tribute3 "$@"
