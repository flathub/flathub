#!/bin/sh

export PYTHONPATH="/app/opt/trelby/:/app/opt/trelby/bin:/app/opt/trelby/src"

exec "/app/opt/trelby/bin/trelby" "$@"
