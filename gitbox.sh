#!/bin/sh
# Launcher installed as /app/bin/gitbox (the manifest's `command`).
#
# zypak-wrapper, from org.electronjs.Electron2.BaseApp, redirects Chromium's
# sandbox onto the Flatpak sandbox — that is what replaces the --no-sandbox the
# deb's desktop entry passes, without giving up sandboxing.
#
# --ozone-platform-hint=auto makes Electron pick Wayland when a compositor is
# there and X11 otherwise, matching the --socket=wayland/--socket=fallback-x11
# pair in the manifest.
exec zypak-wrapper /app/gitbox/gitbox --ozone-platform-hint=auto "$@"
