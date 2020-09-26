#!/usr/bin/sh

# File to determine if we already showed the mimic strategy warning before
mimic_stamp="$XDG_DATA_HOME/flatpak-brave-mimic-stamp"
if [ ! -f "$mimic_stamp" ] && ! zypak-helper spawn-strategy-test; then
	zenity --info --title='Brave Flatpak' --no-wrap --text="$(< /app/share/flatpak-brave/mimic_warning.txt)"
	> "$mimic_stamp"
elif [ -f "$mimic_stamp" ]; then
	rm -f "$mimic_stamp"
fi

export TMPDIR="$XDG_RUNTIME_DIR/app/$FLATPAK_ID"
exec zypak-wrapper.sh /app/extra/brave-browser "$@"
