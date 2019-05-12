# write winecfg launcher script
# USAGE: write_bin_winecfg
# NEEDED VARS: APP_POSTRUN APP_PRERUN CONFIG_DIRS CONFIG_FILES DATA_DIRS DATA_FILES GAME_ID (LANG) PATH_BIN PATH_GAME PKG (PKG_PATH)
# CALLS: write_bin
# CALLED BY: write_bin
write_bin_winecfg() {
	if [ "$winecfg_launcher" != '1' ]; then
		winecfg_launcher='1'
		APP_WINECFG_ID="${GAME_ID}_winecfg"
		APP_WINECFG_TYPE='wine'
		APP_WINECFG_EXE='winecfg'
		write_bin 'APP_WINECFG'
		local target
		target="${pkg_path}${PATH_BIN}/$APP_WINECFG_ID"
		sed --in-place 's/# Run the game/# Run WINE configuration/' "$target"
		sed --in-place 's/^cd "$PATH_PREFIX"//'                     "$target"
		sed --in-place 's/wine "$APP_EXE" $APP_OPTIONS $@/winecfg/' "$target"
	fi
}

# write launcher script - set WINE-specific prefix-specific vars
# USAGE: write_bin_set_wine
# CALLED BY: write_bin
write_bin_set_wine() {
	local winearch
	case "$app_type" in
		('wine'|'wine-staging')
			use_archive_specific_value "${PKG}_ARCH"
			local architecture
			architecture="$(get_value "${PKG}_ARCH")"
			case "$architecture" in
				('32') winearch='win32' ;;
				('64') winearch='win64' ;;
			esac
		;;
		('wine32'|'wine32-staging') winearch='win32' ;;
		('wine64'|'wine64-staging') winearch='win64' ;;
	esac
	cat >> "$file" <<- EOF
	WINEARCH='$winearch'
	export WINEARCH
	EOF
	cat >> "$file" <<- 'EOF'
	WINEDEBUG='-all'
	export WINEDEBUG
	WINEDLLOVERRIDES='winemenubuilder.exe,mscoree,mshtml=d'
	export WINEDLLOVERRIDES
	WINEPREFIX="$PATH_PREFIX"
	export WINEPREFIX
	# Work around WINE bug 41639
	FREETYPE_PROPERTIES="truetype:interpreter-version=35"
	export FREETYPE_PROPERTIES

	EOF
}

# write launcher script - set WINE-specific user-writable directories
# USAGE: write_bin_build_wine
# NEEDED VARS: APP_WINETRICKS
# CALLED BY: write_bin
write_bin_build_wine() {
	cat >> "$file" <<- 'EOF'
	# Build prefix
	if ! [ -e "$WINEPREFIX/dosdevices" ]; then
	  mkdir "$WINEPREFIX/dosdevices"
	  ln -s .. "$WINEPREFIX/dosdevices/c:"
	  # Use LANG=C to avoid localized directory names
	  LANG=C wineboot --init 2>/dev/null
	EOF

	if ! { [ $version_major_target -lt 2 ] || [ $version_minor_target -lt 8 ] ; }; then
		cat >> "$file" <<- 'EOF'
		  # Remove most links pointing outside of the WINE prefix
		  find "$WINEPREFIX/users/$(whoami)" -type l | while read directory; do
		    rm "$directory"
		    mkdir "$directory"
		  done
		EOF
	fi

	if [ "$APP_WINETRICKS" ]; then
		cat >> "$file" <<- EOF
		  winetricks $APP_WINETRICKS
		  sleep 1s
		EOF
	fi

	if [ "$APP_REGEDIT" ]; then
		cat >> "$file" <<- EOF
		  for reg_file in $APP_REGEDIT; do
		EOF
		cat >> "$file" <<- 'EOF'
		  (
		    cd "$WINEPREFIX/drive_c/"
		    cp "$PATH_GAME/$reg_file" .
		    reg_file_basename="${reg_file##*/}"
		    wine regedit "$reg_file_basename"
		    rm "$reg_file_basename"
		  )
		  done
		EOF
	fi

	cat >> "$file" <<- 'EOF'
	fi
	EOF
}

# write launcher script - run the WINE game
# USAGE: write_bin_run_wine
# CALLED BY: write_bin
write_bin_run_wine() {
	cat >> "$file" <<- 'EOF'
	# Run the game

	cd "$PATH_PREFIX"
	EOF

	cat >> "$file" <<- EOF
	$app_prerun
	wine "\$APP_EXE" \$APP_OPTIONS \$@
	$app_postrun
	EOF
}

# write winecfg menu entry
# USAGE: write_desktop_winecfg
# NEEDED VARS: (LANG) PATH_DESK PKG (PKG_PATH)
# CALLS: write_desktop
# CALLED BY: write_desktop
write_desktop_winecfg() {
	local pkg_path
	pkg_path="$(get_value "${PKG}_PATH")"
	[ -n "$pkg_path" ] || missing_pkg_error 'write_desktop_winecfg' "$PKG"
	APP_WINECFG_ID="${GAME_ID}_winecfg"
	APP_WINECFG_NAME="$GAME_NAME - WINE configuration"
	APP_WINECFG_CAT='Settings'
	write_desktop 'APP_WINECFG'
	sed --in-place 's/Icon=.\+/Icon=winecfg/' "${pkg_path}${PATH_DESK}/${APP_WINECFG_ID}.desktop"
}

