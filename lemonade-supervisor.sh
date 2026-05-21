#!/usr/bin/env bash
# Orchestrates lemond + lemonade-tray + lemonade-app inside the Lemonade flatpak.

set -u

# `:= default` form so tests can pre-export alternative paths.
: "${LEMONADE_BIN_LEMOND:=/app/bin/lemond}"
: "${LEMONADE_BIN_TRAY:=/app/bin/lemonade-tray}"
: "${LEMONADE_BIN_APP:=/app/bin/lemonade-app}"

LEMOND_OWNER=""
LEMOND_PID=""
TRAY_PID=""
APP_PID=""

# Logs land in $XDG_RUNTIME_DIR (ephemeral, cleared on logout) co-located with
# lemond's own server log. log() also tees to stderr so a failed file write
# still leaves a trail visible to `flatpak run` / journald.
log_file=""
init_logging() {
  local dir="${LEMONADE_LOG_DIR:-${XDG_RUNTIME_DIR:-/tmp}/lemonade}"
  if mkdir -p "$dir" 2>/dev/null; then
    log_file="$dir/supervisor.log"
  fi
}
log() {
  local stamp
  stamp="$(date -u +%FT%TZ)"
  printf '%s lemonade-supervisor: %s\n' "$stamp" "$*" >&2
  if [ -n "$log_file" ]; then
    printf '%s %s\n' "$stamp" "$*" >>"$log_file" 2>/dev/null || true
  fi
}

# Two-tier: env override, else per-app private cache. We deliberately don't
# probe host paths (/var/lib/lemonade, ~/.cache/lemonade) — a running host
# lemond is caught by external-detection, which never reaches this function.
resolve_data_root() {
  if [ -n "${LEMONADE_DATA_DIR:-}" ]; then
    DATA_SOURCE=env
    DATA_ROOT="$LEMONADE_DATA_DIR"
  else
    DATA_SOURCE=bundled
    DATA_ROOT="${XDG_CACHE_HOME:-$HOME/.cache}/lemonade"
  fi
}

print_data_resolution() {
  echo "LEMONADE_DATA_SOURCE=$DATA_SOURCE"
  echo "LEMONADE_DATA_ROOT=$DATA_ROOT"
}

# A wildcard bind (0.0.0.0 / :: / empty) is reached over loopback, not via the
# wildcard itself; map it to 127.0.0.1. Concrete addresses pass through.
connect_target() {
  case "$1" in
    0.0.0.0|::|"") echo 127.0.0.1 ;;
    *) echo "$1" ;;
  esac
}

# Exit codes (consumed by main): 0 = external lemond reachable; 1 = none found;
# 2 = caller set FORCE_BUNDLED (skip detection).
detect_host_lemond() {
  if [ "${LEMONADE_FLATPAK_FORCE_BUNDLED:-}" = "1" ]; then
    return 2
  fi
  local host port
  host="$(connect_target "${LEMONADE_HOST:-127.0.0.1}")"
  port="${LEMONADE_PORT:-13305}"
  if curl --silent --fail --max-time 0.5 \
       "http://${host}:${port}/api/v1/health" >/dev/null 2>&1; then
    LEMOND_HOST="$host"
    LEMOND_PORT="$port"
    return 0
  fi
  # Stop at the direct check: don't auto-adopt a beacon-advertised server.
  if [ "${LEMONADE_FLATPAK_NO_BEACON:-}" = "1" ]; then
    return 1
  fi
  # Beacon fallback for a host lemond on a non-default port. Python (in the
  # GNOME runtime) keeps us off external socat/nc dependencies.
  local beacon
  beacon="$(python3 -c '
import json, socket, sys
try:
    s = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    s.bind(("0.0.0.0", 13305))
    s.settimeout(1.0)
    data, _ = s.recvfrom(4096)
    s.close()
    msg = json.loads(data.decode("utf-8"))
    print(msg.get("host", "127.0.0.1"), msg.get("port", 13305))
except Exception:
    sys.exit(1)
' 2>/dev/null)"
  if [ -n "$beacon" ]; then
    LEMOND_HOST="${beacon% *}"
    LEMOND_PORT="${beacon#* }"
    if curl --silent --fail --max-time 0.5 \
         "http://${LEMOND_HOST}:${LEMOND_PORT}/api/v1/health" >/dev/null 2>&1; then
      return 0
    fi
  fi
  return 1
}

# lemond's CLI is `lemond [CACHE_DIR] --port P --host H` (no `serve` subcommand,
# no --cache-dir flag). Pass --port/--host explicitly so the health probe and
# tray target the address lemond binds, regardless of any config.json values.
start_bundled_lemond() {
  local port host connect_host
  port="${LEMONADE_PORT:-13305}"
  host="${LEMONADE_HOST:-127.0.0.1}"
  connect_host="$(connect_target "$host")"

  # Point HF_HOME at the host's HF cache (granted via xdg-cache/huggingface:rw)
  # so we don't re-download models already available to other AI tools. Respect
  # an existing setting.
  if [ -z "${HF_HOME:-}" ]; then
    export HF_HOME="$HOME/.cache/huggingface"
    log "HF_HOME=$HF_HOME (sharing user host HF cache)"
  fi

  log "starting bundled lemond on ${host}:${port} (data root $DATA_ROOT source $DATA_SOURCE)"
  "$LEMONADE_BIN_LEMOND" "$DATA_ROOT" --port "$port" --host "$host" \
    >>"$log_file" 2>&1 &
  LEMOND_PID=$!
  LEMOND_HOST="$connect_host"
  LEMOND_PORT="$port"
  LEMOND_OWNER=flatpak
  for i in $(seq 1 30); do
    if curl --silent --fail --max-time 0.5 \
         "http://${connect_host}:${port}/api/v1/health" >/dev/null 2>&1; then
      log "bundled lemond ready (pid $LEMOND_PID)"
      return 0
    fi
    sleep 0.5
  done
  log "bundled lemond failed to come up within 15s"
  kill "$LEMOND_PID" 2>/dev/null || true
  return 1
}

start_tray() {
  log "starting tray (host=$LEMOND_HOST port=$LEMOND_PORT)"
  "$LEMONADE_BIN_TRAY" --host "$LEMOND_HOST" --port "$LEMOND_PORT" \
    >>"$log_file" 2>&1 &
  TRAY_PID=$!
  # Tray exit within 1s = init failure (no StatusNotifierWatcher; e.g. stock
  # GNOME without AppIndicator). Fall back to app-as-anchor mode.
  sleep 1
  if ! kill -0 "$TRAY_PID" 2>/dev/null; then
    wait "$TRAY_PID" 2>/dev/null || true
    log "tray exited during startup; running without tray (app close = cleanup)"
    TRAY_PID=""
  fi
}

# Bounded escalation HTTP /internal/shutdown -> SIGTERM -> SIGKILL so the
# supervisor always exits within ~15s of being asked to clean up.
graceful_shutdown_owned_lemond() {
  [ "$LEMOND_OWNER" = "flatpak" ] || return 0
  [ -n "$LEMOND_PID" ] || return 0
  log "tray gone; shutting down owned lemond (pid $LEMOND_PID)"

  curl --silent --max-time 5 -X POST \
    "http://${LEMOND_HOST}:${LEMOND_PORT}/internal/shutdown" >/dev/null 2>&1 || true
  for i in $(seq 1 10); do
    kill -0 "$LEMOND_PID" 2>/dev/null || { log "owned lemond exited (graceful)"; LEMOND_PID=""; return 0; }
    sleep 0.5
  done

  log "owned lemond still up after graceful; SIGTERM (pid $LEMOND_PID)"
  kill -TERM "$LEMOND_PID" 2>/dev/null || true
  for i in $(seq 1 6); do
    kill -0 "$LEMOND_PID" 2>/dev/null || { log "owned lemond exited (SIGTERM)"; LEMOND_PID=""; return 0; }
    sleep 0.5
  done

  log "owned lemond ignored SIGTERM; SIGKILL (pid $LEMOND_PID)"
  kill -KILL "$LEMOND_PID" 2>/dev/null || true
  LEMOND_PID=""
}

on_exit() {
  stop_app_gracefully
  if [ -n "$TRAY_PID" ]; then
    kill -TERM "$TRAY_PID" 2>/dev/null || true
    wait "$TRAY_PID" 2>/dev/null || true
  fi
  graceful_shutdown_owned_lemond
}

stop_app_gracefully() {
  [ -n "$APP_PID" ] || return 0
  kill -0 "$APP_PID" 2>/dev/null || { APP_PID=""; return 0; }
  log "closing desktop app (pid $APP_PID)"
  kill -TERM "$APP_PID" 2>/dev/null || true
  for i in $(seq 1 10); do
    kill -0 "$APP_PID" 2>/dev/null || { APP_PID=""; return 0; }
    sleep 0.5
  done
  kill -KILL "$APP_PID" 2>/dev/null || true
  APP_PID=""
}

# Second-instance: background-launch the app in this sandbox, then block on a
# SHARED flock on the supervisor's lock file. When the supervisor exits and
# releases its exclusive lock, every shared waiter across every sandbox wakes
# at once and tears down its app. Cross-sandbox PID namespaces don't matter —
# the kernel does the signalling via flock.
second_instance_sidecar() {
  log "lock held; running as sidecar"
  "$LEMONADE_BIN_APP" "$@" >>"${log_file:-/dev/null}" 2>&1 &
  APP_PID=$!
  # `flock <file> -c true` opens its own fresh fd internally; the file-path
  # form (vs `flock 8`) avoids inheriting sidecar bash's fd 9, which is
  # already open to this file and can confuse per-OFD lock tracking.
  flock -s "$SUPERVISOR_LOCK" -c true &
  local waiter_pid=$!
  wait -n "$APP_PID" "$waiter_pid" 2>/dev/null || true
  if ! kill -0 "$APP_PID" 2>/dev/null; then
    kill "$waiter_pid" 2>/dev/null || true
    wait "$waiter_pid" 2>/dev/null || true
    APP_PID=""
    return 0
  fi
  log "supervisor exited; closing this sandbox's app (pid $APP_PID)"
  stop_app_gracefully
}

SUPERVISOR_LOCK=""

acquire_lock() {
  SUPERVISOR_LOCK="$XDG_RUNTIME_DIR/app/$FLATPAK_ID/supervisor.lock"
  mkdir -p "$(dirname "$SUPERVISOR_LOCK")"
  exec 9>"$SUPERVISOR_LOCK"
  if ! flock -x -n 9; then
    # Another supervisor instance holds the lock — run as a sidecar that
    # owns this sandbox's app and tears it down when the supervisor exits.
    second_instance_sidecar "$@"
    exit 0
  fi
}

log_prelude() {
  log "PRELUDE owner=$LEMOND_OWNER host=$LEMOND_HOST:$LEMOND_PORT data_source=$DATA_SOURCE data_root=$DATA_ROOT pid=$$ lemond_pid=${LEMOND_PID:-none} tray_pid=${TRAY_PID:-none} app_pid=${APP_PID:-none}"
}

usage() {
  cat >&2 <<'EOF'
lemonade-supervisor — Lemonade flatpak orchestrator
Usage:
  lemonade-supervisor [APP_ARGS...]      # full launch (detect/start lemond, tray, app)
  lemonade-supervisor --print-data-resolution
  lemonade-supervisor --detect-host
EOF
}

main() {
  resolve_data_root
  init_logging
  # Sidecars run their entire app lifecycle inside acquire_lock, so the EXIT
  # trap MUST be registered before it. The explicit signal trap forces
  # `exit` (which fires EXIT) instead of bash's default terminate-without-
  # cleanup — INT/TERM cover typical kills, HUP covers session/terminal
  # teardown (gnome-session logout, terminal close with huponexit).
  trap 'exit' INT TERM HUP
  trap on_exit EXIT

  case "${1:-}" in
    --print-data-resolution)
      print_data_resolution
      exit 0
      ;;
    --detect-host)
      detect_host_lemond
      _rc=$?
      if [ "$_rc" -eq 0 ]; then
        echo "LEMOND_HOST=$LEMOND_HOST"
        echo "LEMOND_PORT=$LEMOND_PORT"
      fi
      exit "$_rc"
      ;;
    --*)
      usage
      exit 64
      ;;
  esac

  mkdir -p "${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
  export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

  acquire_lock "$@"

  if detect_host_lemond; then
    LEMOND_OWNER=external
    log "detected external lemond at $LEMOND_HOST:$LEMOND_PORT"
  else
    rc=$?
    if [ "$rc" -eq 2 ] || [ "$rc" -eq 1 ]; then
      start_bundled_lemond || { log "fatal: bundled lemond failed"; exit 1; }
    else
      log "unexpected detect rc=$rc"; exit 1
    fi
  fi

  start_tray
  log_prelude

  "$LEMONADE_BIN_APP" "$@" >>"$log_file" 2>&1 &
  APP_PID=$!

  if [ -z "$TRAY_PID" ]; then
    # No tray: app close triggers cleanup; any sidecars are anchored on this
    # supervisor's exclusive lock and unblock when we exit.
    wait "$APP_PID" 2>/dev/null || true
    APP_PID=""
    exit 0
  fi

  # Wait for whichever of {tray, app} exits first. App close leaves tray +
  # lemond up (re-open via a sidecar `flatpak run`); tray quit drops the
  # exclusive lock — which wakes every sidecar waiter across every sandbox.
  while :; do
    if [ -n "$APP_PID" ]; then
      wait -n "$TRAY_PID" "$APP_PID" 2>/dev/null || true
    else
      wait "$TRAY_PID" 2>/dev/null || true
    fi
    if ! kill -0 "$TRAY_PID" 2>/dev/null; then
      TRAY_PID=""
      break
    fi
    APP_PID=""
    log "desktop app closed; tray still active"
  done

  stop_app_gracefully
}

main "$@"
