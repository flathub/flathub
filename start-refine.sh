#!/usr/bin/env bash

IFS=: read -ra host_data_dirs < <(flatpak-spawn --host sh -c 'echo "$XDG_DATA_DIRS"')

# To avoid potentially muddying up $XDG_DATA_DIRS too much, we link the schema paths
# into a temporary directory.
bridge_dir=$XDG_RUNTIME_DIR/dconf-bridge
mkdir -p "$bridge_dir"

HOST_XDG_DATA_DIRS=""

for dir in "${host_data_dirs[@]}"; do
  if [[ "$dir" == /usr/* ]]; then
    dir=/run/host/"$dir"
  fi

  schemas="$dir/glib-2.0/schemas"
  if [[ -d "$schemas" ]]; then
    bridged=$(mktemp -d XXXXXXXXXX -p "$bridge_dir")
    mkdir -p "$bridged"/glib-2.0
    ln -s "$schemas" "$bridged"/glib-2.0
    HOST_XDG_DATA_DIRS="${HOST_XDG_DATA_DIRS}:${bridged}"
  fi
done

# We MUST prepend the host's data dirs BEFORE the Flatpak environment's own dirs,
# otherwise data (such as default values) load in the wrong order and would then
# incorrectly prefer the Flatpak's internal defaults instead of the host's defaults!
if [[ ! -z "${HOST_XDG_DATA_DIRS}" ]]; then
  XDG_DATA_DIRS="${HOST_XDG_DATA_DIRS:1}:${XDG_DATA_DIRS}"
fi

export XDG_DATA_DIRS
exec refine "$@"
