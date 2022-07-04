#!/bin/bash

export WINEPREFIX="/var/data/tntconnect"
export WINEARCH="win32"
export WINEDLLOVERRIDES="msado15=n,b"
export XDG_CACHE_HOME="/app/tmp"

VERSION_NUM="4.0.32"
VERSION_FILE="${WINEPREFIX}/com.tntware.TntConnect.version"

TNTCONNECT_SETUP=/app/bin/SetupTntConnect.exe
RUN_DIR="${WINEPREFIX}/drive_c/Program Files/TntWare/TntConnect"
RUN_CMD="TntConnect.exe"
WINE="/app/bin/wine"

declare -ra WINE_PACKAGES=(jet40 mdac28 msxml6 corefonts)

set_wine_settings() {
	echo "Installing wine requirements."

	for package in ${WINE_PACKAGES[@]}; do
		echo "Installing $package in wine"
		exec 3> >(zenity --progress --auto-close --pulsate --title "Setup TntConnect" --text "Installing $package in wine" )
		winetricks --unattended $package
		exec 3>&-
	done
}

# Run only if TntConnect isn't installed
first_run() {
	mkdir -p $WINEPREFIX

	set_wine_settings

	echo "${VERSION_NUM}" > "${VERSION_FILE}"

	echo "Running TntConnect installer."
	exec 3> >(zenity --progress --auto-close --pulsate --title "Setup TntConnect" --text "Running TntConnect installer" )
	winetricks --unattended $package
	"${WINE}" "${TNTCONNECT_SETUP}" /s
	exec 3>&-

	echo "Setup finished."
	echo
}

is_updated() {
	if [ -f "${VERSION_FILE}" ]; then
		last_version="$(cat ${VERSION_FILE})"
	else
		last_version="0"
	fi

	echo "${VERSION_NUM}" > "${VERSION_FILE}"

	if [[ "${VERSION_NUM}" == "${last_version}" ]]; then
		return 0
	else
		return 1
	fi
}

# Main function
startup() {
	if [ ! -f "$RUN_DIR/$RUN_CMD" ]; then
		echo "First run of TntConnect."
		first_run
	else
		if ! is_updated; then
			echo "Not up to date, re-run wine settings"
			set_wine_settings
		fi
	fi

	echo "Starting TntConnect..."
	cd "$RUN_DIR"
	"${WINE}" "${RUN_CMD}"
}

startup
