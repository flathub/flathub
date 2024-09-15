#!/bin/sh

export PYTHONPATH="/app/opt/trelby-2.4.10/:/app/opt/trelby-2.4.10/bin:/app/opt/trelby-2.4.10/src"

exec "/app/opt/trelby-2.4.10/bin/trelby" "$@"
