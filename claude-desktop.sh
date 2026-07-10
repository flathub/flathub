#!/bin/sh
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
exec zypak-wrapper /app/extra/claude-desktop/claude-desktop \
	--ozone-platform-hint=auto \
	--enable-wayland-ime \
	--wayland-text-input-version=3 \
	"$@"
