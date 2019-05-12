# prepare package layout by putting files from archive in the right packages
# directories
# USAGE: prepare_package_layout [$pkg…]
# NEEDED VARS: (LANG) (PACKAGES_LIST) PLAYIT_WORKDIR (PKG_PATH)
prepare_package_layout() {
	if [ -z "$1" ]; then
		[ -n "$PACKAGES_LIST" ] || prepare_package_layout_error_no_list
		prepare_package_layout $PACKAGES_LIST
		return 0
	fi
	for package in "$@"; do
		PKG="$package"
		organize_data "GAME_${PKG#PKG_}" "$PATH_GAME"
		organize_data "DOC_${PKG#PKG_}"  "$PATH_DOC"
		for i in $(seq 0 9); do
			organize_data "GAME${i}_${PKG#PKG_}" "$PATH_GAME"
			organize_data "DOC${i}_${PKG#PKG_}"  "$PATH_DOC"
		done
	done
}

# display an error when calling prepare_package_layout() without argument while
# $PACKAGES_LIST is unset or empty
# USAGE: prepare_package_layout_error_no_list
# NEEDED VARS: (LANG)
prepare_package_layout_error_no_list() {
	print_error
	case "${LANG%_*}" in
		('fr')
			string='prepare_package_layout ne peut pas être appelé sans argument si $PACKAGES_LIST n’est pas défini.'
		;;
		('en'|*)
			string='prepare_package_layout can not be called without argument if $PACKAGES_LIST is not set.'
		;;
	esac
	printf '%s\n' "$string"
	return 1
}

# put files from archive in the right package directories
# USAGE: organize_data $id $path
# NEEDED VARS: (LANG) PLAYIT_WORKDIR (PKG) (PKG_PATH)
organize_data() {
	[ -n "$PKG" ] || organize_data_error_missing_pkg
	if [ "$OPTION_ARCHITECTURE" != all ] && [ -n "${PACKAGES_LIST##*$PKG*}" ]; then
		skipping_pkg_warning 'organize_data' "$PKG"
		return 0
	fi
	local pkg_path
	if [ "$DRY_RUN" = '1' ]; then
		pkg_path="$(get_value "${PKG}_PATH")"
		[ -n "$pkg_path" ] || missing_pkg_error 'organize_data' "$PKG"
		return 0
	fi
	use_archive_specific_value "ARCHIVE_${1}_PATH"
	use_archive_specific_value "ARCHIVE_${1}_FILES"
	local archive_path
	archive_path="$(get_value "ARCHIVE_${1}_PATH")"
	local archive_files
	archive_files="$(get_value "ARCHIVE_${1}_FILES")"

	if [ "$archive_path" ] && [ "$archive_files" ] && [ -d "$PLAYIT_WORKDIR/gamedata/$archive_path" ]; then
		pkg_path="$(get_value "${PKG}_PATH")"
		[ -n "$pkg_path" ] || missing_pkg_error 'organize_data' "$PKG"
		pkg_path="${pkg_path}$2"
		mkdir --parents "$pkg_path"
		(
			cd "$PLAYIT_WORKDIR/gamedata/$archive_path"
			for file in $archive_files; do
				if [ -e "$file" ]; then
					cp --recursive --force --link --parents --no-dereference --preserve=links "$file" "$pkg_path"
					rm --recursive "$file"
				fi
			done
		)
	fi
}

# display an error when calling organize_data() with $PKG unset or empty
# USAGE: organize_data_error_missing_pkg
# NEEDED VARS: (LANG)
organize_data_error_missing_pkg() {
	print_error
	case "${LANG%_*}" in
		('fr')
			string='organize_data ne peut pas être appelé si $PKG n’est pas défini.\n'
		;;
		('en'|*)
			string='organize_data can not be called if $PKG is not set.\n'
		;;
	esac
	printf "$string"
	return 1
}

