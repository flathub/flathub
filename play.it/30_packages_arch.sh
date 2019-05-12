# write .pkg.tar package meta-data
# USAGE: pkg_write_arch
# NEEDED VARS: GAME_NAME PKG_DEPS_ARCH
# CALLED BY: write_metadata
pkg_write_arch() {
	local pkg_deps
	if [ "$(get_value "${pkg}_DEPS")" ]; then
		pkg_set_deps_arch $(get_value "${pkg}_DEPS")
	fi
	use_archive_specific_value "${pkg}_DEPS_ARCH"
	if [ "$(get_value "${pkg}_DEPS_ARCH")" ]; then
		pkg_deps="$pkg_deps $(get_value "${pkg}_DEPS_ARCH")"
	fi
	local pkg_size
	pkg_size=$(du --total --block-size=1 --summarize "$pkg_path" | tail --lines=1 | cut --fields=1)
	local target
	target="$pkg_path/.PKGINFO"

	PKG="$pkg"
	get_package_version

	mkdir --parents "${target%/*}"

	cat > "$target" <<- EOF
	pkgname = $pkg_id
	pkgver = $PKG_VERSION
	packager = $pkg_maint
	builddate = $(date +"%m%d%Y")
	size = $pkg_size
	arch = $pkg_architecture
	EOF

	if [ -n "$pkg_description" ]; then
		cat >> "$target" <<- EOF
		pkgdesc = $GAME_NAME - $pkg_description - ./play.it script version $script_version
		EOF
	else
		cat >> "$target" <<- EOF
		pkgdesc = $GAME_NAME - ./play.it script version $script_version
		EOF
	fi

	for dep in $pkg_deps; do
		cat >> "$target" <<- EOF
		depend = $dep
		EOF
	done

	if [ -n "$pkg_provide" ]; then
		cat >> "$target" <<- EOF
		conflict = $pkg_provide
		provides = $pkg_provide
		EOF
	fi

	target="$pkg_path/.INSTALL"

	if [ -e "$postinst" ]; then
		cat >> "$target" <<- EOF
		post_install() {
		$(cat "$postinst")
		}

		post_upgrade() {
		post_install
		}
		EOF
	fi

	if [ -e "$prerm" ]; then
		cat >> "$target" <<- EOF
		pre_remove() {
		$(cat "$prerm")
		}

		pre_upgrade() {
		pre_remove
		}
		EOF
	fi
}

# set list or Arch Linux dependencies from generic names
# USAGE: pkg_set_deps_arch $dep[…]
# CALLS: pkg_set_deps_arch32 pkg_set_deps_arch64
# CALLED BY: pkg_write_arch
pkg_set_deps_arch() {
	use_archive_specific_value "${pkg}_ARCH"
	local architecture
	architecture="$(get_value "${pkg}_ARCH")"
	case $architecture in
		('32')
			pkg_set_deps_arch32 "$@"
		;;
		('64')
			pkg_set_deps_arch64 "$@"
		;;
	esac
}

# set list or Arch Linux 32-bit dependencies from generic names
# USAGE: pkg_set_deps_arch32 $dep[…]
# CALLED BY: pkg_set_deps_arch
pkg_set_deps_arch32() {
	for dep in "$@"; do
		case $dep in
			('alsa')
				pkg_dep='lib32-alsa-lib lib32-alsa-plugins'
			;;
			('bzip2')
				pkg_dep='lib32-bzip2'
			;;
			('dosbox')
				pkg_dep='dosbox'
			;;
			('freetype')
				pkg_dep='lib32-freetype2'
			;;
			('gcc32')
				pkg_dep='gcc-multilib lib32-gcc-libs'
			;;
			('gconf')
				pkg_dep='lib32-gconf'
			;;
			('glibc')
				pkg_dep='lib32-glibc'
			;;
			('glu')
				pkg_dep='lib32-glu'
			;;
			('glx')
				pkg_dep='lib32-libgl'
			;;
			('gtk2')
				pkg_dep='lib32-gtk2'
			;;
			('json')
				pkg_dep='lib32-json-c'
			;;
			('libcurl')
				pkg_dep='lib32-curl'
			;;
			('libcurl-gnutls')
				pkg_dep='lib32-libcurl-gnutls'
			;;
			('libstdc++')
				pkg_dep='lib32-gcc-libs'
			;;
			('libudev1')
				pkg_dep='lib32-systemd'
			;;
			('libxrandr')
				pkg_dep='lib32-libxrandr'
			;;
			('nss')
				pkg_dep='lib32-nss'
			;;
			('openal')
				pkg_dep='lib32-openal'
			;;
			('pulseaudio')
				pkg_dep='pulseaudio'
			;;
			('sdl1.2')
				pkg_dep='lib32-sdl'
			;;
			('sdl2')
				pkg_dep='lib32-sdl2'
			;;
			('sdl2_image')
				pkg_dep='lib32-sdl2_image'
			;;
			('sdl2_mixer')
				pkg_dep='lib32-sdl2_mixer'
			;;
			('theora')
				pkg_dep='lib32-libtheora'
			;;
			('vorbis')
				pkg_dep='lib32-libvorbis'
			;;
			('wine'|'wine32'|'wine64')
				pkg_dep='wine'
			;;
			('wine-staging'|'wine32-staging'|'wine64-staging')
				pkg_dep='wine-staging'
			;;
			('winetricks')
				pkg_dep='winetricks'
			;;
			('xcursor')
				pkg_dep='lib32-libxcursor'
			;;
			('xft')
				pkg_dep='lib32-libxft'
			;;
			('xgamma')
				pkg_dep='xorg-xgamma'
			;;
			('xrandr')
				pkg_dep='xorg-xrandr'
			;;
			(*)
				pkg_deps="$dep"
			;;
		esac
		pkg_deps="$pkg_deps $pkg_dep"
	done
}

# set list or Arch Linux 64-bit dependencies from generic names
# USAGE: pkg_set_deps_arch64 $dep[…]
# CALLED BY: pkg_set_deps_arch
pkg_set_deps_arch64() {
	for dep in "$@"; do
		case $dep in
			('alsa')
				pkg_dep='alsa-lib alsa-plugins'
			;;
			('bzip2')
				pkg_dep='bzip2'
			;;
			('dosbox')
				pkg_dep='dosbox'
			;;
			('freetype')
				pkg_dep='freetype2'
			;;
			('gcc32')
				pkg_dep='gcc-multilib lib32-gcc-libs'
			;;
			('gconf')
				pkg_dep='gconf'
			;;
			('glibc')
				pkg_dep='glibc'
			;;
			('glu')
				pkg_dep='glu'
			;;
			('glx')
				pkg_dep='libgl'
			;;
			('gtk2')
				pkg_dep='gtk2'
			;;
			('json')
				pkg_dep='json-c'
			;;
			('libcurl')
				pkg_dep='curl'
			;;
			('libcurl-gnutls')
				pkg_dep='libcurl-gnutls'
			;;
			('libstdc++')
				pkg_dep='gcc-libs'
			;;
			('libudev1')
				pkg_dep='libsystemd'
			;;
			('libxrandr')
				pkg_dep='libxrandr'
			;;
			('nss')
				pkg_dep='nss'
			;;
			('openal')
				pkg_dep='openal'
			;;
			('pulseaudio')
				pkg_dep='pulseaudio'
			;;
			('sdl1.2')
				pkg_dep='sdl'
			;;
			('sdl2')
				pkg_dep='sdl2'
			;;
			('sdl2_image')
				pkg_dep='sdl2_image'
			;;
			('sdl2_mixer')
				pkg_dep='sdl2_mixer'
			;;
			('theora')
				pkg_dep='libtheora'
			;;
			('vorbis')
				pkg_dep='libvorbis'
			;;
			('wine'|'wine32'|'wine64')
				pkg_dep='wine'
			;;
			('winetricks')
				pkg_dep='winetricks'
			;;
			('xcursor')
				pkg_dep='libxcursor'
			;;
			('xft')
				pkg_dep='libxft'
			;;
			('xgamma')
				pkg_dep='xorg-xgamma'
			;;
			('xrandr')
				pkg_dep='xorg-xrandr'
			;;
			(*)
				pkg_dep="$dep"
			;;
		esac
		pkg_deps="$pkg_deps $pkg_dep"
	done
}

# build .pkg.tar package
# USAGE: pkg_build_arch $pkg_path
# NEEDED VARS: (OPTION_COMPRESSION) (LANG) PLAYIT_WORKDIR
# CALLS: pkg_print
# CALLED BY: build_pkg
pkg_build_arch() {
	local pkg_filename
	pkg_filename="$PWD/${1##*/}.pkg.tar"

	if [ -e "$pkg_filename" ]; then
		pkg_build_print_already_exists "${pkg_filename##*/}"
		eval ${pkg}_PKG=\"$pkg_filename\"
		export ${pkg}_PKG
		return 0
	fi

	local tar_options
	tar_options='--create --group=root --owner=root'

	case $OPTION_COMPRESSION in
		('gzip')
			tar_options="$tar_options --gzip"
			pkg_filename="${pkg_filename}.gz"
		;;
		('xz')
			tar_options="$tar_options --xz"
			pkg_filename="${pkg_filename}.xz"
		;;
		('bzip2')
			tar_options="$tar_options --bzip2"
			pkg_filename="${pkg_filename}.bz2"
		;;
		('none') ;;
		(*)
			liberror 'OPTION_COMPRESSION' 'pkg_build_arch'
		;;
	esac

	pkg_print "${pkg_filename##*/}"
	if [ "$DRY_RUN" = '1' ]; then
		printf '\n'
		eval ${pkg}_PKG=\"$pkg_filename\"
		export ${pkg}_PKG
		return 0
	fi

	(
		cd "$1"
		local files
		files='.PKGINFO *'
		if [ -e '.INSTALL' ]; then
			files=".INSTALL $files"
		fi
		tar $tar_options --file "$pkg_filename" $files
	)

	eval ${pkg}_PKG=\"$pkg_filename\"
	export ${pkg}_PKG

	print_ok
}

