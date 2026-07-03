#!/bin/bash
# Slicer Flatpak wrapper.
#
# Problem: Slicer (Slicer_STORE_SETTINGS_IN_APPLICATION_HOME_DIR) always
# reads/writes extension settings relative to $SLICER_HOME:
#   extension settings: $SLICER_HOME/slicer.org/Slicer-<rev>.ini
#   default ext dir:    $SLICER_HOME/slicer.org/Extensions-<rev>
#
# In the Flatpak, SLICER_HOME=/app/opt/Slicer (read-only), so we cannot
# write there. Changing SLICER_HOME entirely breaks Python (which looks for
# scripts at $SLICER_HOME/bin/Python/slicer/slicerqt.py), SSL certs, etc.
#
# Solution: build a writable SLICER_HOME that is a thin shell over the real
# install — symlinks for bin/, lib/, share/, etc. pointing back into
# /app/opt/Slicer/, with ONLY slicer.org/ as a real writable directory.
# Pass this new home via --launcher-additional-settings so SlicerApp-real
# reads launcher settings (via the symlinked bin/) and finds everything, but
# resolves extension paths inside the writable slicer.org/.

SLICER_INSTALL="/app/opt/Slicer"
LAUNCHER_SETTINGS="$SLICER_INSTALL/bin/SlicerLauncherSettings.ini"
REVISION=$(grep "^revision=" "$LAUNCHER_SETTINGS" 2>/dev/null | cut -d= -f2 | tr -d '[:space:]')
REVISION="${REVISION:-34621}"

SLICER_HOME_OVERRIDE="${XDG_DATA_HOME:-$HOME/.local/share}/Slicer"
SETTINGS_DIR="$SLICER_HOME_OVERRIDE/slicer.org"
EXTENSIONS_DIR="$SETTINGS_DIR/Extensions-${REVISION}"

# Create symlinks from the writable SLICER_HOME to the real install for
# everything that must remain read-only / version-locked.
mkdir -p "$SLICER_HOME_OVERRIDE"
for item in bin include lib libexec resources share Slicer; do
    link="$SLICER_HOME_OVERRIDE/$item"
    target="$SLICER_INSTALL/$item"
    if [ ! -e "$link" ] && [ -e "$target" ]; then
        ln -sf "$target" "$link"
    fi
done

# slicer.org/ is the ONLY real writable directory.
mkdir -p "$SETTINGS_DIR" "$EXTENSIONS_DIR"

# Migrate general settings on first run (Slicer.ini must exist here so that
# Slicer calls QSettings::setPath and writes preferences to this dir).
SLICER_INI="$SETTINGS_DIR/Slicer.ini"
XDG_INI="${XDG_CONFIG_HOME:-$HOME/.config}/slicer.org/Slicer.ini"
if [ ! -f "$SLICER_INI" ]; then
    if [ -f "$XDG_INI" ]; then cp "$XDG_INI" "$SLICER_INI"
    else touch "$SLICER_INI"; fi
fi

# Ensure extension settings file exists with InstallPath set.
REV_INI="$SETTINGS_DIR/Slicer-${REVISION}.ini"
touch "$REV_INI"
if ! awk '/^\[Extensions\]/{f=1;next}/^\[/{f=0}f&&/^InstallPath=/{exit 0}END{exit 1}' \
        "$REV_INI" 2>/dev/null; then
    if grep -q '^\[Extensions\]' "$REV_INI" 2>/dev/null; then
        sed -i "/^\[Extensions\]$/a InstallPath=$EXTENSIONS_DIR" "$REV_INI"
    else
        printf '\n[Extensions]\nInstallPath=%s\n' "$EXTENSIONS_DIR" >> "$REV_INI"
    fi
fi

# Per-launch additional settings that override SLICER_HOME.
OVERRIDE_INI="$(mktemp /tmp/slicer-launcher-XXXXXX.ini)"
printf '[EnvironmentVariables]\nSLICER_HOME=%s\n' "$SLICER_HOME_OVERRIDE" > "$OVERRIDE_INI"
trap "rm -f '$OVERRIDE_INI'" EXIT

export LD_LIBRARY_PATH="/app/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

exec "$SLICER_INSTALL/Slicer" \
    --launcher-additional-settings "$OVERRIDE_INI" \
    "$@"
