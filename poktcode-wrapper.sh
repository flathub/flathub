#!/bin/bash
# PoktCode Flatpak launcher — uses zypak from Electron BaseApp.
set -eu

export TMPDIR="${XDG_RUNTIME_DIR:-/tmp}/app/${FLATPAK_ID:-com.poktweb.poktcode}"
mkdir -p "$TMPDIR"

# Integrated terminal bridge to host shell
if [ -x "/app/bin/poktcode-host-shell" ]; then
  export THEIA_SHELL="/app/bin/poktcode-host-shell"
  export SHELL="/app/bin/poktcode-host-shell"
fi

# Chromium nested sandbox inside Flatpak often aborts with SIGTRAP (exit 133).
export ELECTRON_DISABLE_SANDBOX="${ELECTRON_DISABLE_SANDBOX:-1}"

# Avoid blocking on KWallet/portal password backends during startup (KDE6).
export ELECTRON_PASSWORD_STORE="${ELECTRON_PASSWORD_STORE:-basic}"

# Transparent alwaysOnTop splash is unstable on Plasma Wayland + NVIDIA.
export THEIA_NO_SPLASH="${THEIA_NO_SPLASH:-1}"

# Avoid GPU process sandbox under zypak (extra SIGTRAP source on some hosts).
export ZYPAK_DISABLE_GPU_SANDBOX="${ZYPAK_DISABLE_GPU_SANDBOX:-1}"

# Run file watcher in-process. A forked parcel-watcher under zypak pins the
# backend at 100% CPU and freezes the window (can't close).
export POKT_WATCHER_SINGLE_THREADED=1

# Plasma + NVIDIA: Wayland congela o renderer na splash / janela vazia.
# ELECTRON_OZONE_PLATFORM_HINT=x11 é ignorado pelo Electron 42 quando há Wayland —
# é preciso passar --ozone-platform=x11. Se não houver DISPLAY, cai para Wayland.
OZONE_ARGS=()
if [ -n "${DISPLAY:-}" ]; then
  export ELECTRON_OZONE_PLATFORM_HINT=x11
  OZONE_ARGS=(--ozone-platform=x11)
elif [ -n "${WAYLAND_DISPLAY:-}" ]; then
  export ELECTRON_OZONE_PLATFORM_HINT=wayland
  OZONE_ARGS=(--ozone-platform=wayland)
fi

APP_DIR="/app/lib/com.poktweb.poktcode"
BIN="$APP_DIR/poktcode.bin"

if [ ! -x "$BIN" ]; then
  if [ -x "$APP_DIR/poktcode" ] && [ ! -L "$APP_DIR/poktcode" ]; then
    BIN="$APP_DIR/poktcode"
  elif [ -x "$APP_DIR/electron" ]; then
    BIN="$APP_DIR/electron"
  else
    echo "PoktCode: executável Electron não encontrado em $APP_DIR" >&2
    exit 1
  fi
fi

# Stale SingletonLock after a crash makes relaunches look like "won't open".
CFG="${XDG_CONFIG_HOME:-$HOME/.config}/PoktCode"
if [ -e "$CFG/SingletonLock" ] || [ -L "$CFG/SingletonLock" ]; then
  if ! pgrep -x poktcode.bin >/dev/null 2>&1; then
    rm -f "$CFG/SingletonLock" "$CFG/SingletonCookie" "$CFG/SingletonSocket" 2>/dev/null || true
  fi
fi

exec zypak-wrapper "$BIN" --no-sandbox --password-store=basic --disable-gpu-shader-disk-cache \
  "${OZONE_ARGS[@]}" \
  "$@"
