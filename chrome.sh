#!/usr/bin/bash

# Merge the policies with the host ones.
policy_root=/etc/opt/chrome/policies

for policy_type in managed recommended enrollment; do
  policy_dir="$policy_root/$policy_type"
  mkdir -p "$policy_dir"

  if [[ -d "/run/host/$policy_root/$policy_type" ]]; then
    find "/run/host/$policy_root/$policy_type" -type f -name '*' \
      -exec ln -sf '{}' "$policy_root/$policy_type" \;
  fi
done

touch "${XDG_CONFIG_HOME}/google-chrome/WidevineCdm"

exec cobalt "$@"