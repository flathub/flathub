# select package architecture to build
# USAGE: select_package_architecture
# NEEDED_VARS: OPTION_ARCHITECTURE PACKAGES_LIST
# CALLS: select_package_architecture_warning_unavailable select_package_architecture_error_unknown select_package_architecture_warning_unsupported
select_package_architecture() {
	[ "$OPTION_ARCHITECTURE" = 'all' ] && return 0
	local version_major_target
	local version_minor_target
	version_major_target="${target_version%%.*}"
	version_minor_target=$(printf '%s' "$target_version" | cut --delimiter='.' --fields=2)
	if [ $version_major_target -lt 2 ] || [ $version_minor_target -lt 6 ]; then
		select_package_architecture_warning_unsupported
		OPTION_ARCHITECTURE='all'
		export OPTION_ARCHITECTURE
		return 0
	fi
	if [ "$OPTION_ARCHITECTURE" = 'auto' ]; then
		case "$(uname --machine)" in
			('i686')
				OPTION_ARCHITECTURE='32'
			;;
			('x86_64')
				OPTION_ARCHITECTURE='64'
			;;
			(*)
				select_package_architecture_warning_unknown
				OPTION_ARCHITECTURE='all'
				export OPTION_ARCHITECTURE
				return 0
			;;
		esac
		export OPTION_ARCHITECTURE
		select_package_architecture
		return 0
	fi
	local package_arch
	local packages_list_32
	local packages_list_64
	local packages_list_all
	for package in $PACKAGES_LIST; do
		package_arch="$(get_value "${package}_ARCH")"
		case "$package_arch" in
			('32')
				packages_list_32="$packages_list_32 $package"
			;;
			('64')
				packages_list_64="$packages_list_64 $package"
			;;
			(*)
				packages_list_all="$packages_list_all $package"
			;;
		esac
	done
	case "$OPTION_ARCHITECTURE" in
		('32')
			if [ -z "$packages_list_32" ]; then
				select_package_architecture_warning_unavailable
				OPTION_ARCHITECTURE='all'
				return 0
			fi
			PACKAGES_LIST="$packages_list_32 $packages_list_all"
		;;
		('64')
			if [ -z "$packages_list_64" ]; then
				select_package_architecture_warning_unavailable
				OPTION_ARCHITECTURE-'all'
				return 0
			fi
			PACKAGES_LIST="$packages_list_64 $packages_list_all"
		;;
		(*)
			select_package_architecture_error_unknown
		;;
	esac
	export PACKAGES_LIST
}

# display an error if selected architecture is not available
# USAGE: select_package_architecture_warning_unavailable
# NEEDED_VARS: (LANG) OPTION_ARCHITECTURE
# CALLED_BY: select_package_architecture
select_package_architecture_warning_unavailable() {
	local string
	case "${LANG%_*}" in
		('fr')
			string='L’architecture demandée n’est pas disponible : %s\n'
		;;
		('en'|*)
			string='Selected architecture is not available: %s\n'
		;;
	esac
	print_warning
	printf "$string" "$OPTION_ARCHITECTURE"
}

# display an error if selected architecture is not supported
# USAGE: select_package_architecture_error_unknown
# NEEDED_VARS: (LANG) OPTION_ARCHITECTURE
# CALLED_BY: select_package_architecture
select_package_architecture_error_unknown() {
	local string
	case "${LANG%_*}" in
		('fr')
			string='L’architecture demandée n’est pas supportée : %s\n'
		;;
		('en'|*)
			string='Selected architecture is not supported: %s\n'
		;;
	esac
	print_error
	printf "$string" "$OPTION_ARCHITECTURE"
	exit 1
}

# display a warning if using --architecture on a pre-2.6 script
# USAGE: select_package_architecture_warning_unsupported
# NEEDED_VARS: (LANG)
# CALLED_BY: select_package_architecture
select_package_architecture_warning_unsupported() {
	local string
	case "${LANG%_*}" in
		('fr')
			string='L’option --architecture n’est pas gérée par ce script.'
		;;
		('en'|*)
			string='--architecture option is not supported by this script.'
		;;
	esac
	print_warning
	printf '%s\n\n' "$string"
}

