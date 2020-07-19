#!/usr/bin/bash

policy_root=/etc/opt/chrome/policies

for policy_type in managed recommended; do
  policy_dir="$policy_root/$policy_type"
  mkdir -p "$policy_dir"

  if [[ "$policy_type" == 'managed' ]]; then
    ln -sf /app/share/flatpak-chrome/flatpak_policy.json "$policy_dir"
  fi

  if [[ -d "/run/host/$policy_root/$policy_type" ]]; then
    find "/run/host/$policy_root/$policy_type" -name '*.json' \
      -exec ln -s '{}' "$policy_root/$policy_type" \;
  fi
done

run_stamp="$XDG_DATA_HOME/flatpak-chrome-run-stamp"
mimic_stamp="$XDG_DATA_HOME/flatpak-chrome-mimic-stamp"

if [[ ! -f "$run_stamp" ]]; then
  set /app/share/flatpak-chrome/first_run.html chrome://welcome "$@"
  touch "$run_stamp"
fi

if [[ ! -f "$mimic_stamp" ]] && ! zypak-helper spawn-strategy-test; then
  zenity --info --title='Chrome Flatpak' --no-wrap \
    --text="$(< /app/share/flatpak-chrome/mimic_warning.txt)"
  touch "$mimic_stamp"
fi

if [[ -f "$XDG_CONFIG_HOME/chrome-flags.conf" ]]; then
  IFS=$'\n'
  flags=($(grep -v '^#' "$XDG_CONFIG_HOME/chrome-flags.conf"))
  unset IFS

  set -- "${flags[@]}" "$@"
fi

export TMPDIR="$XDG_CACHE_HOME"
exec zypak-wrapper.sh /app/extra/chrome "$@"
