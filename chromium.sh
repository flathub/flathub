#!/bin/bash

# Reference: https://github.com/flathub/io.github.ungoogled_software.ungoogled_chromium/blob/master/chromium.sh

merge_extensions() {
	(
		shopt -s nullglob
		dest="/app/helium/extensions/$1"
		mkdir -p "${dest}"
		for ext in "/app/helium/${1%/*}/$1/"*; do
			ln -s "${ext}" "${dest}"
		done
	)
}

if [[ ! -f "/app/helium/extensions/no-mount-stamp" ]]; then
	# Merge all legacy extension points if the symlinks
	# had a tmpfs mounted over them.
	merge_extensions "native-messaging-hosts"
	merge_extensions "policies/managed"
	merge_extensions "policies/recommended"
fi

export TMPDIR="${XDG_RUNTIME_DIR}/app/${FLATPAK_ID}"

exec zypak-wrapper /app/helium/chrome "$@"
