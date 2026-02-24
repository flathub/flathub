#!/bin/sh
set -eu

backend="${PICOCRYPT_BACKEND:-auto}"

case "$backend" in
wayland)
	exec /app/bin/picocrypt-ng-wayland "$@"
	;;
x11)
	exec /app/bin/picocrypt-ng-x11 "$@"
	;;
auto)
	if [ -n "${WAYLAND_DISPLAY:-}" ]; then
		exec /app/bin/picocrypt-ng-wayland "$@"
	fi
	if [ -n "${DISPLAY:-}" ]; then
		exec /app/bin/picocrypt-ng-x11 "$@"
	fi
	if [ "$#" -gt 0 ]; then
		# Allow CLI subcommands in headless/CI shells where DISPLAY is absent.
		exec /app/bin/picocrypt-ng-x11 "$@"
	fi
	echo "Picocrypt-NG: no display detected (WAYLAND_DISPLAY/DISPLAY unset)." >&2
	echo "Set PICOCRYPT_BACKEND=x11|wayland to force backend selection." >&2
	exit 1
	;;
*)
	echo "Picocrypt-NG: invalid PICOCRYPT_BACKEND value '$backend'." >&2
	echo "Expected one of: auto, x11, wayland." >&2
	exit 2
	;;
esac
