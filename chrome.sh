#!/usr/bin/bash

policy_dir=/etc/opt/chrome/policies/managed
mkdir -p "$policy_dir"
ln -sf /app/share/flatpak-chrome/flatpak_policy.json "$policy_dir"

run_stamp="$XDG_DATA_HOME/flatpak-chrome-run-stamp"
mimic_stamp="$XDG_DATA_HOME/flatpak-chrome-mimic-stamp"

if [[ ! -f "$run_stamp" ]]; then
  set /app/share/flatpak-chrome/first_run.html "$@"
  touch "$run_stamp"
fi

if [[ ! -f "$mimic_stamp" ]] && ! zypak-helper spawn-strategy-test; then
  zenity --info --title='Chrome Flatpak' --no-wrap \
    --text="$(< /app/share/flatpak-chrome/mimic_warning.txt)"
  touch "$mimic_stamp"
fi

exec zypak-wrapper.sh /app/extra/chrome "$@"
