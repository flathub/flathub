#!/bin/sh
export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"
# Chromium only autodetects a keyring on GNOME/KDE; force libsecret so
# sign-in survives a restart on other desktops too
exec zypak-wrapper /app/extra/claude-desktop/claude-desktop --password-store=gnome-libsecret "$@"
