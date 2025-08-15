#!/bin/bash
exec env XDG_CONFIG_HOME="$HOME/.config" XDG_DATA_HOME="$HOME/.local/share" /app/extra/toolbox/bin/jetbrains-toolbox "$@" 