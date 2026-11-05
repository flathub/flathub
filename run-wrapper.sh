#!/usr/bin/env bash
set -euo pipefail

# Detect whether `--password-store` arg was given explicitly.
has_password_store_override=0
for arg in "$@"; do
  if [[ "$arg" == --password-store || "$arg" == --password-store=* ]]; then
    has_password_store_override=1
    break
  fi
done

# Query DBus for whether an interface with the given name is present.
has_dbus_interface() {
  # Bail out if DBus is not available.
  command -v gdbus >/dev/null 2>&1 || return 1
  local interface_name="$1"

  # Query owned interfaces.
  if gdbus call --session \
       --dest org.freedesktop.DBus \
       --object-path /org/freedesktop/DBus \
       --method org.freedesktop.DBus.ListNames \
       2>/dev/null | grep -Fq "'$interface_name'"; then
    return 0
  fi

  # Query activatable interfaces as well.
  gdbus call --session \
    --dest org.freedesktop.DBus \
    --object-path /org/freedesktop/DBus \
    --method org.freedesktop.DBus.ListActivatableNames \
    2>/dev/null | grep -Fq "'$interface_name'"
}

# Detect available password store (only if not overridden).
detected_password_store=()
if [[ $has_password_store_override -eq 0 ]]; then
  if has_dbus_interface org.freedesktop.secrets; then
    detected_password_store=(--password-store=gnome-libsecret)
  elif has_dbus_interface org.kde.kwalletd6; then
    detected_password_store=(--password-store=kwallet6)
  elif has_dbus_interface org.kde.kwalletd5; then
    detected_password_store=(--password-store=kwallet5)
  fi
fi

exec /app/main/ThreemaDesktopLauncher \
  --launcher-target-bin /app/bin/zypak-wrapper.sh \
  /app/main/ThreemaDesktop \
  "${detected_password_store[@]}" \
  "$@"
