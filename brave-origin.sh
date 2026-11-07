#!/bin/sh

# Merge the policies with the host ones.
for proot in "etc/brave/policies" "etc/static/brave/policies"; do
  for ptype in managed recommended enrollment; do
    if [ -d "/run/host/$proot/$ptype" ]; then
      mkdir -p "/etc/brave/policies/$ptype"
      ln -sf "/run/host/$proot/$ptype"/*.json "/etc/brave/policies/$ptype" 2>/dev/null
    fi
  done
done

exec cobalt "$@" --no-default-browser-check
