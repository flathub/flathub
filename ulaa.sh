#!/usr/bin/bash

# Merge the policies with the host ones.
policy_root=/etc/opt/zoho/ulaa/policies

for policy_type in managed recommended enrollment; do
  policy_dir="$policy_root/$policy_type"
  mkdir -p "$policy_dir"

  if [[ -d "/run/host/$policy_root/$policy_type" ]]; then
    find "/run/host/$policy_root/$policy_type" -maxdepth 1 \
      -type f -name '*' -name '*.json' -type f \
      -exec ln -sf '{}' "$policy_root/$policy_type" \;
  fi
done

exec cobalt "$@"
