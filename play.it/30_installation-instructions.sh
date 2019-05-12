# print installation instructions
# USAGE: print_instructions $pkg[…]
# NEEDED VARS: (GAME_NAME) (OPTION_PACKAGE) (PACKAGES_LIST)
print_instructions() {
	[ "$GAME_NAME" ] || return 1
	if [ $# = 0 ]; then
		print_instructions $PACKAGES_LIST
		return 0
	fi
	local package_arch
	local packages_list_32
	local packages_list_64
	local packages_list_all
	local string
	case "$OPTION_PACKAGE" in
		('appdir')
			print_instructions_appdir
			return 0
		;;
	esac
	for package in "$@"; do
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
	case "${LANG%_*}" in
		('fr')
			string='\nInstallez %s en lançant la série de commandes suivantes en root :\n'
		;;
		('en'|*)
			string='\nInstall %s by running the following commands as root:\n'
		;;
	esac
	printf "$string" "$GAME_NAME"
	if [ -n "$packages_list_32" ] && [ -n "$packages_list_64" ]; then
		print_instructions_architecture_specific '32' $packages_list_all $packages_list_32
		print_instructions_architecture_specific '64' $packages_list_all $packages_list_64
	else
		case $OPTION_PACKAGE in
			('arch')
				print_instructions_arch "$@"
			;;
			('deb')
				print_instructions_deb "$@"
			;;
			('flatpak')
				echo Flatpak done!
			;;
			(*)
				liberror 'OPTION_PACKAGE' 'print_instructions'
			;;
		esac
	fi
	printf '\n'
}

# print installation instructions for Arch Linux - 32-bit version
# USAGE: print_instructions_architecture_specific $pkg[…]
# CALLS: print_instructions_arch print_instructions_deb
print_instructions_architecture_specific() {
	case "${LANG%_*}" in
		('fr')
			string='\nversion %s-bit :\n'
		;;
		('en'|*)
			string='\n%s-bit version:\n'
		;;
	esac
	printf "$string" "$1"
	shift 1
	case $OPTION_PACKAGE in
		('arch')
			print_instructions_arch "$@"
		;;
		('deb')
			print_instructions_deb "$@"
		;;
		(*)
			liberror 'OPTION_PACKAGE' 'print_instructions'
		;;
	esac
}

