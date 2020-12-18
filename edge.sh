#!/usr/bin/bash

# Merge the policies with the host ones.
policy_root=/etc/opt/chrome/policies

for policy_type in managed recommended; do
  policy_dir="$policy_root/$policy_type"
  mkdir -p "$policy_dir"

  if [[ "$policy_type" == 'managed' ]]; then
    ln -sf /app/share/flatpak-edge/flatpak_policy.json "$policy_dir"
  fi

  if [[ -d "/run/host/$policy_root/$policy_type" ]]; then
    find "/run/host/$policy_root/$policy_type" -name '*.json' \
      -exec ln -sf '{}' "$policy_root/$policy_type" \;
  fi
done

# Determine if to show the first run page & the mimic strategy warning.
# run_stamp="$XDG_DATA_HOME/flatpak-edge-run-stamp"
mimic_stamp="$XDG_DATA_HOME/flatpak-edge-mimic-stamp"

if [[ ! -f "$mimic_stamp" ]] && ! zypak-helper spawn-strategy-test; then
  zenity --info --title='Edge Flatpak' --no-wrap \
    --text="$(< /app/share/flatpak-edge/mimic_warning.txt)"
  touch "$mimic_stamp"
fi

if [[ -f "$XDG_CONFIG_HOME/chrome-flags.conf" ]]; then
  IFS=$'\n'
  flags=($(grep -v '^#' "$XDG_CONFIG_HOME/chrome-flags.conf"))
  unset IFS

  set -- "${flags[@]}" "$@"
fi

# Chrome loads cursors by itself, following the standard XCursor search
# directories. However, the fd.o runtime patches XCursor to look in
# $XDG_DATA_DIRS, but Chrome's own loading of course does not follow that.
# Therefore, we manually set the XCursor path to follow $XDG_DATA_DIRS here.
export XCURSOR_PATH=$(echo "$XDG_DATA_DIRS" | sed 's,\(:\|$\),/icons\1,g')
export CHROME_WRAPPER=$(readlink -f "$0")
export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"

exec zypak-wrapper.sh /app/extra/msedge "$@"
