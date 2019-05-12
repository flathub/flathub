# write package meta-data
# USAGE: write_metadata [$pkg…]
# NEEDED VARS: (ARCHIVE) GAME_NAME (OPTION_PACKAGE) PACKAGES_LIST (PKG_ARCH) PKG_DEPS_ARCH PKG_DEPS_DEB PKG_DESCRIPTION PKG_ID (PKG_PATH) PKG_PROVIDE
# CALLS: liberror pkg_write_arch pkg_write_deb set_architecture testvar
write_metadata() {
	case "$OPTION_PACKAGE" in
		('appdir')
			return 0
		;;
		('flatpak')
			install -Dp "${BASH_SOURCE%/*}"/{[0-9]*,libplayit2.sh} "$0" -t "$PATH_DESK/play.it"
			SHA256=$(find -L ~/.local/share/flatpak/extra-data/ -maxdepth 2 -samefile "$SOURCE_ARCHIVE" -printf %h -quit)
			if [ -z "$SHA256" ]; then
				SHA256=$(sha256sum "$SOURCE_ARCHIVE")
				SHA256=${SHA256%% *}
				mkdir ~/.local/share/flatpak/extra-data/$SHA256
				ln -sr "$SOURCE_ARCHIVE" ~/.local/share/flatpak/extra-data/$SHA256/
			else
				SHA256=${SHA256##*/}
			fi

			eval "cat << HERE
$(cat ${BASH_SOURCE%/*}/flatpak/manifest.json)
HERE"			> "${PATH_DESK}/it.dotslashplay.${GAME_ID}.json"
			eval "cat << HERE
$(cat ${BASH_SOURCE%/*}/flatpak/appdata.xml)
HERE"			> "${PATH_DESK}/it.dotslashplay.${GAME_ID}.appdata.xml"
			flatpak-builder --force-clean --repo=repo build-dir "${PATH_DESK}/it.dotslashplay.${GAME_ID}.json"
			flatpak build-update-repo repo
			flatpak remote-add --no-gpg-verify --if-not-exists --user test repo
			flatpak update --appstream test
			#flatpak-builder --force-clean build-dir "${PATH_DESK}/it.dotslashplay.${GAME_ID}.json"
			#flatpak-builder --force-clean --install --user build-dir "${PATH_DESK}/it.dotslashplay.${GAME_ID}.json"
			return
			;;
	esac

	if [ $# = 0 ]; then
		write_metadata $PACKAGES_LIST
		return 0
	fi
	local pkg_architecture
	local pkg_description
	local pkg_id
	local pkg_maint
	local pkg_path
	local pkg_provide
	for pkg in "$@"; do
		testvar "$pkg" 'PKG' || liberror 'pkg' 'write_metadata'
		if [ "$OPTION_ARCHITECTURE" != all ] && [ -n "${PACKAGES_LIST##*$pkg*}" ]; then
			skipping_pkg_warning 'write_metadata' "$pkg"
			continue
		fi

		# Set package-specific variables
		set_architecture "$pkg"
		pkg_id="$(get_value "${pkg}_ID")"
		pkg_maint="$(whoami)@$(hostname)"
		pkg_path="$(get_value "${pkg}_PATH")"
		[ -n "$pkg_path" ] || missing_pkg_error 'write_metadata' "$pkg"
		[ "$DRY_RUN" = '1' ] && continue
		pkg_provide="$(get_value "${pkg}_PROVIDE")"

		use_archive_specific_value "${pkg}_DESCRIPTION"
		pkg_description="$(get_value "${pkg}_DESCRIPTION")"

		case $OPTION_PACKAGE in
			('arch')
				pkg_write_arch
			;;
			('deb')
				pkg_write_deb
			;;
			(*)
				liberror 'OPTION_PACKAGE' 'write_metadata'
			;;
		esac
	done
	rm  --force "$postinst" "$prerm"
}

# build .pkg.tar or .deb package
# USAGE: build_pkg [$pkg…]
# NEEDED VARS: (OPTION_COMPRESSION) (LANG) (OPTION_PACKAGE) PACKAGES_LIST (PKG_PATH) PLAYIT_WORKDIR
# CALLS: liberror pkg_build_arch pkg_build_deb testvar
build_pkg() {
	if [ $# = 0 ]; then
		build_pkg $PACKAGES_LIST
		return 0
	fi
	local pkg_path
	for pkg in "$@"; do
		testvar "$pkg" 'PKG' || liberror 'pkg' 'build_pkg'
		if [ "$OPTION_ARCHITECTURE" != all ] && [ -n "${PACKAGES_LIST##*$pkg*}" ]; then
			skipping_pkg_warning 'build_pkg' "$pkg"
			return 0
		fi
		pkg_path="$(get_value "${pkg}_PATH")"
		[ -n "$pkg_path" ] || missing_pkg_error 'build_pkg' "$PKG"
		case $OPTION_PACKAGE in
			('appdir'|'flatpak')
				# No package is built in AppDir mode
				true
			;;
			('arch')
				pkg_build_arch "$pkg_path"
			;;
			('deb')
				pkg_build_deb "$pkg_path"
			;;
			(*)
				liberror 'OPTION_PACKAGE' 'build_pkg'
			;;
		esac
	done
}

# print package building message
# USAGE: pkg_print $file
# NEEDED VARS: (LANG)
# CALLED BY: pkg_build_arch pkg_build_deb
pkg_print() {
	local string
	case "${LANG%_*}" in
		('fr')
			string='Construction de %s'
		;;
		('en'|*)
			string='Building %s'
		;;
	esac
	printf "$string" "$1"
}

# print package building message
# USAGE: pkg_build_print_already_exists $file
# NEEDED VARS: (LANG)
# CALLED BY: pkg_build_arch pkg_build_deb
pkg_build_print_already_exists() {
	local string
	case "${LANG%_*}" in
		('fr')
			string='%s existe déjà.\n'
		;;
		('en'|*)
			string='%s already exists.\n'
		;;
	esac
	printf "$string" "$1"
}

# guess package format to build from host OS
# USAGE: packages_guess_format $variable_name
# NEEDED VARS: (LANG) DEFAULT_OPTION_PACKAGE
packages_guess_format() {
	local guessed_host_os
	local variable_name
	eval variable_name=\"$1\"
	if [ -e '/etc/os-release' ]; then
		guessed_host_os="$(grep '^ID=' '/etc/os-release' | cut --delimiter='=' --fields=2)"
	elif command -v lsb_release >/dev/null 2>&1; then
		guessed_host_os="$(lsb_release --id --short | tr '[:upper:]' '[:lower:]')"
	fi
	case "$guessed_host_os" in
		('debian'|\
		 'ubuntu'|\
		 'linuxmint'|\
		 'handylinux')
			eval $variable_name=\'deb\'
		;;
		('arch'|\
		 'manjaro'|'manjarolinux')
			eval $variable_name=\'arch\'
		;;
		(*)
			print_warning
			case "${LANG%_*}" in
				('fr')
					string1='L’auto-détection du format de paquet le plus adapté a échoué.\n'
					string2='Le format de paquet %s sera utilisé par défaut.\n'
				;;
				('en'|*)
					string1='Most pertinent package format auto-detection failed.\n'
					string2='%s package format will be used by default.\n'
				;;
			esac
			printf "$string1"
			printf "$string2" "$DEFAULT_OPTION_PACKAGE"
			printf '\n'
		;;
	esac
	export $variable_name
}

