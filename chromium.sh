#!/bin/bash

get_int32_property() {
  gdbus call -e \
    -d org.freedesktop.portal.Flatpak \
    -o /org/freedesktop/portal/Flatpak \
    -m org.freedesktop.DBus.Properties.Get \
    org.freedesktop.portal.Flatpak "$1" \
    | awk 'match($0, /uint32 ([0-9]+)/, m){print m[1];}'
}

# Check the portal version & make sure it supports expose-pids.
if [[ $(get_int32_property version) -lt 4 || \
      $(($(get_int32_property supports) & 1)) -eq 0 ]]; then
  zenity --info --title='Chromium Flatpak' --no-wrap \
    --text="$(< /app/share/flatpak-chromium/portal_error.txt)"
  exit 1
fi

# Merge the flags.
if [[ -f "$XDG_CONFIG_HOME/chromium-flags.conf" ]]; then
  IFS=$'\n'
  flags=($(grep -v '^#' "$XDG_CONFIG_HOME/chromium-flags.conf"))
  unset IFS

  set -- "${flags[@]}" "$@"
fi

export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec /app/chromium/chromium "$@"
