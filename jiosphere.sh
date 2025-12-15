#!/bin/sh

# Merge the policies with the host ones.
for proot in "etc/jiosphere/policies" "etc/static/jiosphere/policies"; do
  for ptype in managed recommended enrollment; do
    if [ -d "/run/host/$proot/$ptype" ]; then
      mkdir -p "/etc/jiosphere/policies/$ptype"
      ln -sf "/run/host/$proot/$ptype"/*.json "/etc/jiosphere/policies/$ptype" 2>/dev/null
    fi
  done
done

# Launch JioSphere
exec zypak-wrapper /app/jiosphere/jiosphere-browser/jiosphere-browser "$@" --no-default-browser-check
#exec zypak-wrapper app/bin/cobalt "$@" --no-default-browser-check
