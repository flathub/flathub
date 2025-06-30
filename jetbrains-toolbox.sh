#!/bin/bash
exec env XDG_CONFIG_HOME="$HOME/.config" XDG_DATA_HOME="$HOME/.local/share" XDG_CACHE_HOME="$HOME/.cache" /app/extra/toolbox/bin/jetbrains-toolbox "$@" 