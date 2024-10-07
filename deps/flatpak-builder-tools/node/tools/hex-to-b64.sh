#!/bin/bash
# Also remove slashes so we can send in _cacache paths.
tr -d '/\n' | xxd -r -p | base64 -w0 | sed 's/$/\n/'
