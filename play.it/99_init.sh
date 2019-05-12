#set -x

if [ "${0##*/}" != 'libplayit2.sh' ] && [ -z "$LIB_ONLY" ]; then

	# Set input field separator to default value (space, tab, newline)
	unset IFS

	# Check library version against script target version

	version_major_library="${library_version%%.*}"
	version_major_target="${target_version%%.*}"

	version_minor_library=$(printf '%s' "$library_version" | cut --delimiter='.' --fields=2)
	version_minor_target=$(printf '%s' "$target_version" | cut --delimiter='.' --fields=2)

	if [ $version_major_library -ne $version_major_target ] || [ $version_minor_library -lt $version_minor_target ]; then
		print_error
		case "${LANG%_*}" in
			('fr')
				string1='Mauvaise version de libplayit2.sh\n'
				string2='La version cible est : %s\n'
			;;
			('en'|*)
				string1='Wrong version of libplayit2.sh\n'
				string2='Target version is: %s\n'
			;;
		esac
		printf "$string1"
		printf "$string2" "$target_version"
		exit 1
	fi

	# Set URL for error messages

	PLAYIT_GAMES_BUG_TRACKER_URL='https://framagit.org/vv221/play.it-games/issues'

	# Set allowed values for common options

	ALLOWED_VALUES_ARCHITECTURE='all 32 64 auto'
	ALLOWED_VALUES_CHECKSUM='none md5'
	ALLOWED_VALUES_COMPRESSION='none gzip xz bzip2'
	ALLOWED_VALUES_PACKAGE='appdir flatpak arch deb'

	# Set default values for common options

	DEFAULT_OPTION_ARCHITECTURE='all'
	DEFAULT_OPTION_CHECKSUM='md5'
	DEFAULT_OPTION_COMPRESSION='none'
	DEFAULT_OPTION_PREFIX='/usr/local'
	DEFAULT_OPTION_PREFIX_APPDIR="${XDG_DATA_HOME:="$HOME/.local/share"}/applications"
	DEFAULT_OPTION_PACKAGE='deb'
	unset winecfg_desktop
	unset winecfg_launcher

	# Parse arguments given to the script

	unset OPTION_ARCHITECTURE
	unset OPTION_CHECKSUM
	unset OPTION_COMPRESSION
	unset OPTION_PREFIX
	unset OPTION_PACKAGE
	unset SOURCE_ARCHIVE
	DRY_RUN='0'

	while [ $# -gt 0 ]; do
		case "$1" in
			('--help')
				help
				exit 0
			;;
			('--architecture='*|\
			 '--architecture'|\
			 '--checksum='*|\
			 '--checksum'|\
			 '--compression='*|\
			 '--compression'|\
			 '--prefix='*|\
			 '--prefix'|\
			 '--package='*|\
			 '--package')
				if [ "${1%=*}" != "${1#*=}" ]; then
					option="$(printf '%s' "${1%=*}" | sed 's/^--//')"
					value="${1#*=}"
				else
					option="$(printf '%s' "$1" | sed 's/^--//')"
					value="$2"
					shift 1
				fi
				if [ "$value" = 'help' ]; then
					eval help_$option
					exit 0
				else
					eval OPTION_$(printf '%s' "$option" | tr '[:lower:]' '[:upper:]')=\"$value\"
					export OPTION_$(printf '%s' "$option" | tr '[:lower:]' '[:upper:]')
				fi
				unset option
				unset value
			;;
			('--dry-run')
				DRY_RUN='1'
				export DRY_RUN
			;;
			('--'*)
				print_error
				case "${LANG%_*}" in
					('fr')
						string='Option inconnue : %s\n'
					;;
					('en'|*)
						string='Unkown option: %s\n'
					;;
				esac
				printf "$string" "$1"
				return 1
			;;
			(*)
				SOURCE_ARCHIVE="$1"
				export SOURCE_ARCHIVE
			;;
		esac
		shift 1
	done

	# Try to detect the host distribution if no package format has been explicitely set

	case "$OPTION_PACKAGE" in
		('appdir')
			if [ "$(id -u)" = '0' ]; then
				print_error
				case "${LANG%_*}" in
					('fr')
						string_error='Le mode AppDir ne doit pas être utilisé via le compte root.\n'
					;;
					('en'|*)
						string_error='AppDir mode is not intended to be run as root.\n'
					;;
				esac
				printf "$string_error"
				exit 1
			fi
			DEFAULT_OPTION_PREFIX="$DEFAULT_OPTION_PREFIX_APPDIR"
		;;
		('flatpak')
			DEFAULT_OPTION_PREFIX="$PWD"
		;;
		('')
			packages_guess_format 'OPTION_PACKAGE'
		;;
	esac

	# Set options not already set by script arguments to default values

	for option in 'ARCHITECTURE' 'CHECKSUM' 'COMPRESSION' 'PREFIX'; do
		if [ -z "$(get_value "OPTION_$option")" ]\
		&& [ -n "$(get_value "DEFAULT_OPTION_$option")" ]; then
			eval OPTION_$option=\"$(get_value "DEFAULT_OPTION_$option")\"
			export OPTION_$option
		fi
	done

	# Check options values validity

	check_option_validity() {
		local name
		name="$1"
		local value
		value="$(get_value "OPTION_$option")"
		local allowed_values
		allowed_values="$(get_value "ALLOWED_VALUES_$option")"
		for allowed_value in $allowed_values; do
			if [ "$value" = "$allowed_value" ]; then
				return 0
			fi
		done
		print_error
		local string1
		local string2
		case "${LANG%_*}" in
			('fr')
				string1='%s n’est pas une valeur valide pour --%s.\n'
				string2='Lancez le script avec l’option --%s=help pour une liste des valeurs acceptés.\n'
			;;
			('en'|*)
				string1='%s is not a valid value for --%s.\n'
				string2='Run the script with the option --%s=help to get a list of supported values.\n'
			;;
		esac
		printf "$string1" "$value" "$(printf '%s' $option | tr '[:upper:]' '[:lower:]')"
		printf "$string2" "$(printf '%s' $option | tr '[:upper:]' '[:lower:]')"
		printf '\n'
		exit 1
	}

	for option in 'CHECKSUM' 'COMPRESSION' 'PACKAGE'; do
		check_option_validity "$option"
	done

	# Do not allow bzip2 compression when building Debian packages

	if
		[ "$OPTION_PACKAGE" = 'deb' ] && \
		[ "$OPTION_COMPRESSION" = 'bzip2' ]
	then
		print_error
		case "${LANG%_*}" in
			('fr')
				string='Le mode de compression bzip2 n’est pas compatible avec la génération de paquets deb.'
			;;
			('en'|*)
				string='bzip2 compression mode is not compatible with deb packages generation.'
			;;
		esac
		printf '%s\n' "$string"
		exit 1
	fi

	# Restrict packages list to target architecture

	select_package_architecture

	# Check script dependencies

	check_deps

	# Set package paths

	case $OPTION_PACKAGE in
		('arch')
			PATH_BIN="$OPTION_PREFIX/bin"
			PATH_DESK='/usr/local/share/applications'
			PATH_DOC="$OPTION_PREFIX/share/doc/$GAME_ID"
			PATH_GAME="$OPTION_PREFIX/share/$GAME_ID"
			PATH_ICON_BASE='/usr/local/share/icons/hicolor'
		;;
		('deb')
			PATH_BIN="$OPTION_PREFIX/games"
			PATH_DESK='/usr/local/share/applications'
			PATH_DOC="$OPTION_PREFIX/share/doc/$GAME_ID"
			PATH_GAME="$OPTION_PREFIX/share/games/$GAME_ID"
			PATH_ICON_BASE='/usr/local/share/icons/hicolor'
		;;
		('appdir')
			PATH_BIN="$OPTION_PREFIX/play.it/$GAME_ID"
			PATH_DESK="$OPTION_PREFIX/play.it/$GAME_ID"
			PATH_DOC="$OPTION_PREFIX/play.it/$GAME_ID/doc"
			PATH_GAME="$OPTION_PREFIX/play.it/$GAME_ID"
			PATH_ICON_BASE="$OPTION_PREFIX/play.it/$GAME_ID/icons/hicolor"
		;;
		('flatpak')
			PATH_DESK="$OPTION_PREFIX/it.dotslashplay.$GAME_ID"
			PATH_ICON_BASE="$PATH_DESK/icons/hicolor"

			PATH_BIN="$PATH_DESK"
			PATH_DOC="$PATH_BIN"
			PATH_GAME="$PATH_BIN"
		;;
		(*)
			liberror 'OPTION_PACKAGE' "$0"
		;;
	esac

	# Set main archive

	archives_get_list
	archive_set_main $ARCHIVES_LIST

	# Set working directories

	set_temp_directories $PACKAGES_LIST

fi
