#!/bin/sh

export PATH=$PATH:/app/tools/tmux/bin
exec /app/tools/tmux/bin/tmux "$@"

