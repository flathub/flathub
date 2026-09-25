#!/usr/bin/env bash
# MedScript OPD - Flatpak Application Launcher
set -e

# Persistent user data directory in Flatpak sandbox
DATA_DIR="${XDG_DATA_HOME:-$HOME/.var/app/io.github.Plaguedoctor_sudo.MedScriptOPD/data}/medscript-opd"
mkdir -p "$DATA_DIR/backups"
chmod -f 700 "$DATA_DIR/backups" 2>/dev/null || true

export DATABASE_PATH="$DATA_DIR/sqlite.db"
export PORT="${PORT:-3000}"
export NODE_ENV="production"

APP_DIR="/app/lib/medscript-opd"
cd "$APP_DIR"

# Seed initial database if first-time run
if [ ! -f "$DATABASE_PATH" ]; then
  echo "[Flatpak] Initializing first-time clinic database at $DATABASE_PATH..."
  npm run db:seed
  chmod -f 600 "$DATABASE_PATH" 2>/dev/null || true
fi

# Background process to launch UI window in system browser
(
  sleep 2
  # In Flatpak sandbox, launch host browser via flatpak-spawn or freedesktop portal
  if command -v flatpak-spawn &>/dev/null; then
    flatpak-spawn --host xdg-open "http://localhost:$PORT" 2>/dev/null || xdg-open "http://localhost:$PORT" 2>/dev/null || true
  elif command -v google-chrome &>/dev/null; then
    google-chrome --app="http://localhost:$PORT" &>/dev/null || true
  elif command -v chromium &>/dev/null; then
    chromium --app="http://localhost:$PORT" &>/dev/null || true
  elif command -v xdg-open &>/dev/null; then
    xdg-open "http://localhost:$PORT" &>/dev/null || true
  fi
) &

# Run Next.js server
exec npm run start -- -p "$PORT" -H 127.0.0.1
