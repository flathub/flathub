#!/bin/bash

merge_extensions() {
  (
    shopt -s nullglob
    dest=/app/chromium/extensions/$1
    mkdir -p $dest
    for ext in /app/chromium/${1%/*}/$1/*; do
      ln -s $ext $dest
    done
  )
}

if [[ ! -f /app/chromium/extensions/no-mount-stamp ]]; then
  # Merge all legacy extension points if the symlinks had a tmpfs mounted over
  # them.
  merge_extensions native-messaging-hosts
  merge_extensions policies/managed
  merge_extensions policies/recommended
fi

export LIBGL_DRIVERS_PATH=/usr/lib/$(uname -m)-linux-gnu/GL/lib/dri
exec cobalt "$@"
