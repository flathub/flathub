#!/usr/bin/env bash
export PYTHONPATH="/app/lib/python3/site-packages:${PYTHONPATH}"
export TERM="${TERM:-xterm-256color}"
export SHELL="${SHELL:-/bin/bash}"
export SSHPILOT_FLATPAK=1
exec python3 -m sshpilot.main "${@}"
