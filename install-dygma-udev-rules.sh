#!/usr/bin/env bash
set -euo pipefail

RULES_PATH="/etc/udev/rules.d/60-dygma.rules"

log() {
  printf '%s\n' "$*"
}

fail() {
  printf 'Error: %s\n' "$*" >&2
  exit 1
}

as_root() {
  if [ "${EUID:-$(id -u)}" -eq 0 ]; then
    "$@"
    return
  fi

  if command -v sudo >/dev/null 2>&1; then
    sudo "$@"
    return
  fi

  if command -v doas >/dev/null 2>&1; then
    doas "$@"
    return
  fi

  fail "This script needs root privileges (run as root, or install sudo/doas)."
}

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || fail "Missing required command: $1"
}

install_rules() {
  local tmp_file
  tmp_file="$(mktemp)"
  trap 'rm -f "$tmp_file"' EXIT

  cat >"$tmp_file" <<'RULES'
# Dygma Raise
SUBSYSTEMS=="usb", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="2200", MODE="0660", TAG+="uaccess"
# bootloader mode
SUBSYSTEMS=="usb", ATTRS{idVendor}=="1209", ATTRS{idProduct}=="2201", MODE="0660", TAG+="uaccess"

# Dygma USB Keyboards Vendor ID
SUBSYSTEMS=="usb", ATTRS{idVendor}=="35ef", MODE="0660", TAG+="uaccess"
# bootloader mode
SUBSYSTEMS=="usb", ATTRS{idVendor}=="35ef", MODE="0660", TAG+="uaccess"

# Dygma HID Keyboards Vendor ID
KERNEL=="hidraw*", ATTRS{idVendor}=="35ef", MODE="0660", TAG+="uaccess"
# bootloader mode
KERNEL=="hidraw*", ATTRS{idVendor}=="35ef", MODE="0660", TAG+="uaccess"
RULES

  as_root mkdir -p /etc/udev/rules.d
  as_root install -m 0644 "$tmp_file" "$RULES_PATH"
  rm -f "$tmp_file"
  trap - EXIT
}

reload_udev() {
  require_cmd udevadm

  as_root udevadm control --reload-rules
  as_root udevadm trigger
}

main() {
  log "Installing Dygma udev rules to $RULES_PATH"
  install_rules
  reload_udev
  log "Done. You may need to unplug and reconnect your keyboard."
}

main "$@"