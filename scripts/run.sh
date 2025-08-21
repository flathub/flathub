#!/usr/bin/env bash
set -euo pipefail
ROOT="/app/extra/current"

# Allow override if you know the exact binary name:
if [ -n "${APP_EXEC:-}" ] && [ -x "$ROOT/$APP_EXEC" ]; then
  exec zypak-wrapper "$ROOT/$APP_EXEC" "$@"
fi

# Try common names
for c in "alderon-games-launcher" "Alderon Games Launcher" "launcher" "com.alderongames.launcher" "electron"; do
  if [ -x "$ROOT/$c" ]; then
    exec zypak-wrapper "$ROOT/$c" "$@"
  fi
done

# Fallback: first top-level executable (ignore chrome-sandbox)
for f in "$ROOT"/*; do
  [ -f "$f" ] && [ -x "$f" ] || continue
  case "$(basename "$f")" in chrome-sandbox|chrome_sandbox) continue ;; esac
  exec zypak-wrapper "$f" "$@"
done

echo "Error: no executable under $ROOT" >&2
exit 1
