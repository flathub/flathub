#!/bin/bash
cd /app/tmm

# Detect architecture and launch appropriate executable
ARCH=$(uname -m)
case "$ARCH" in
    aarch64|arm64)
        exec /app/tmm/tinyMediaManager-arm "$@"
        ;;
    x86_64|amd64)
        exec /app/tmm/tinyMediaManager "$@"
        ;;
    *)
        echo "Unsupported architecture: $ARCH"
        exit 1
        ;;
esac