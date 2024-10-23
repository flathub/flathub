#!/usr/bin/env bash

# Default to dash-qt if no arguments are provided
cmd="dash-qt"

# If the first argument is one of the specific commands, use it instead
if [[ "$1" == "dashd" || "$1" == "dash-cli" || "$1" == "dash-tx" || "$1" == "dash-wallet" ]]; then
  cmd="$1"
  # Remove the command from the arguments list
  shift
fi

# Execute the chosen command with all remaining arguments
exec "/app/bin/internal/$cmd" "$@"

