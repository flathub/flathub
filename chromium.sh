#!/usr/bin/sh

# Show warning for people that don't have expose-pids support every time
if ! zypak-helper spawn-strategy-test; then
	set /app/share/flatpak-chromium/mimic_warning.html chrome://welcome "$@"
fi

export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec zypak-wrapper.sh /app/share/chromium-browser/chromium "$@"
