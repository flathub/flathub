# extract data from given archive
# USAGE: extract_data_from $archive[…]
# NEEDED_VARS: (ARCHIVE) (ARCHIVE_PASSWD) (ARCHIVE_TYPE) (LANG) (PLAYIT_WORKDIR)
# CALLS: liberror extract_7z extract_data_from_print
extract_data_from() {
	[ "$PLAYIT_WORKDIR" ] || return 1
	[ "$ARCHIVE" ] || return 1
	local file
	for file in "$@"; do
		extract_data_from_print "$(basename "$file")"

		local destination
		destination="$PLAYIT_WORKDIR/gamedata"
		mkdir --parents "$destination"
		if [ "$DRY_RUN" = '1' ]; then
			printf '\n'
			return 0
		fi
		local archive_type
		archive_type="$(get_value "${ARCHIVE}_TYPE")"
		case "$archive_type" in
			('7z')
				extract_7z "$file" "$destination"
			;;
			('cabinet')
				cabextract -d "$destination" -q "$file"
				tolower "$destination"
			;;
			('debian')
				dpkg-deb --extract "$file" "$destination"
			;;
			('innosetup'*)
				archive_extraction_innosetup "$archive_type" "$file" "$destination"
			;;
			('msi')
				msiextract --directory "$destination" "$file" 1>/dev/null 2>&1
				tolower "$destination"
			;;
			('mojosetup'|'iso')
				bsdtar --directory "$destination" --extract --file "$file"
				set_standard_permissions "$destination"
			;;
			('nix_stage1')
				local input_blocksize
				input_blocksize=$(head --lines=514 "$file" | wc --bytes | tr --delete ' ')
				dd if="$file" ibs=$input_blocksize skip=1 obs=1024 conv=sync 2>/dev/null | gunzip --stdout | tar --extract --file - --directory "$destination"
			;;
			('nix_stage2')
				tar --extract --xz --file "$file" --directory "$destination"
			;;
			('rar'|'nullsoft-installer')
				# compute archive password from GOG id
				if [ -z "$ARCHIVE_PASSWD" ] && [ -n "$(get_value "${ARCHIVE}_GOGID")" ]; then
					ARCHIVE_PASSWD="$(printf '%s' "$(get_value "${ARCHIVE}_GOGID")" | md5sum | cut -d' ' -f1)"
				fi
				if [ -n "$ARCHIVE_PASSWD" ]; then
					UNAR_OPTIONS="-password $ARCHIVE_PASSWD"
				fi
				unar -no-directory -output-directory "$destination" $UNAR_OPTIONS "$file" 1>/dev/null
			;;
			('tar'|'tar.gz')
				tar --extract --file "$file" --directory "$destination"
			;;
			('zip')
				unzip -d "$destination" "$file" 1>/dev/null
			;;
			('zip_unclean'|'mojosetup_unzip')
				set +o errexit
				unzip -d "$destination" "$file" 1>/dev/null 2>&1
				set -o errexit
				set_standard_permissions "$destination"
			;;
			(*)
				liberror 'ARCHIVE_TYPE' 'extract_data_from'
			;;
		esac

		if [ "${archive_type#innosetup}" = "$archive_type" ]; then
			print_ok
		fi
	done
}

# print data extraction message
# USAGE: extract_data_from_print $file
# NEEDED VARS: (LANG)
# CALLED BY: extract_data_from
extract_data_from_print() {
	case "${LANG%_*}" in
		('fr')
			string='Extraction des données de %s'
		;;
		('en'|*)
			string='Extracting data from %s'
		;;
	esac
	printf "$string" "$1"
}

# extract data from InnoSetup archive
# USAGE: archive_extraction_innosetup $archive_type $archive $destination
# CALLS: archive_extraction_innosetup_error_version
archive_extraction_innosetup() {
	if [ $OPTION_PACKAGE = 'flatpak' ]; then
		local files
		for app in $( echo ${!APP_*} | grep -oP '\w*(?=_ICON)' | uniq ); do
			local list="$(get_value "${app}_ICONS_LIST")"
			[ -n "$list" ] || list="${app}_ICON"
			for icon in $list; do
				use_archive_specific_value "$icon"
				files+=( "-I$(get_value "$icon")" )
			done
		done
	fi

	if [ $OPTION_PACKAGE != 'flatpak' ] || (( ${#files[@]} )); then
		local archive
		local archive_type
		local destination
		local options
		archive_type="$1"
		archive="$2"
		destination="$3"
		options='--progress=1 --silent'
		if [ -n "${archive_type%%*_nolowercase}" ]; then
			options="$options --lowercase"
		fi
		if ( innoextract --list --silent "$archive" 2>&1 1>/dev/null |\
			head --lines=1 |\
			grep --ignore-case 'unexpected setup data version' 1>/dev/null )
		then
			archive_extraction_innosetup_error_version "$archive"
		fi
		printf '\n'
		innoextract $options --extract --output-dir "$destination" "$archive" "${files[@]}"
	fi
}

# print error if available version of innoextract is too low
# USAGE: archive_extraction_innosetup_error_version $archive
# CALLED BY: archive_extraction_innosetup
archive_extraction_innosetup_error_version() {
	local archive
	archive="$1"
	print_error
	case "${LANG%_*}" in
		('fr')
			string='La version de innoextract disponible sur ce système est trop ancienne pour extraire les données de l’archive suivante :'
		;;
		('en'|*)
			string='Available innoextract version is too old to extract data from the following archive:'
		;;
	esac
	printf "$string %s\\n" "$archive"
	exit 1
}

