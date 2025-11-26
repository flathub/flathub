#!/bin/bash
# 設定 Zypak 沙盒環境 (Electron 在 Flatpak 中必須)
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec zypak-wrapper /app/bin/chinese-chess-by-augus "$@"
