# print installation instructions for Debian
# USAGE: print_instructions_deb $pkg[…]
# CALLS: print_instructions_deb_apt print_instructions_deb_dpkg
print_instructions_deb() {
	if command -v apt >/dev/null 2>&1; then
		debian_version="$(apt --version | cut --delimiter=' ' --fields=2)"
		debian_version_major="$(printf '%s' "$debian_version" | cut --delimiter='.' --fields='1')"
		debian_version_minor="$(printf '%s' "$debian_version" | cut --delimiter='.' --fields='2')"
		if [ $debian_version_major -ge 2 ] ||\
		   [ $debian_version_major = 1 ] &&\
		   [ ${debian_version_minor%~*} -ge 1 ]; then
			print_instructions_deb_apt "$@"
		else
			print_instructions_deb_dpkg "$@"
		fi
	else
		print_instructions_deb_dpkg "$@"
	fi
}

# print installation instructions for Debian with apt
# USAGE: print_instructions_deb_apt $pkg[…]
# CALLS: print_instructions_deb_common
# CALLED BY: print_instructions_deb
print_instructions_deb_apt() {
	printf 'apt install'
	print_instructions_deb_common "$@"
}

# print installation instructions for Debian with dpkg + apt-get
# USAGE: print_instructions_deb_dpkg $pkg[…]
# CALLS: print_instructions_deb_common
# CALLED BY: print_instructions_deb
print_instructions_deb_dpkg() {
	printf 'dpkg -i'
	print_instructions_deb_common "$@"
	printf 'apt-get install -f\n'
}

# print installation instructions for Debian (common part)
# USAGE: print_instructions_deb_common $pkg[…]
# CALLED BY: print_instructions_deb_apt print_instructions_deb_dpkg
print_instructions_deb_common() {
	local pkg_path
	local str_format
	for pkg in "$@"; do
		if [ "$OPTION_ARCHITECTURE" != all ] && [ -n "${PACKAGES_LIST##*$pkg*}" ]; then
			skipping_pkg_warning 'print_instructions_deb_common' "$pkg"
			return 0
		fi
		pkg_path="$(get_value "${pkg}_PKG")"
		if [ -z "${pkg_path##* *}" ]; then
			str_format=' "%s"'
		else
			str_format=' %s'
		fi
		printf "$str_format" "$pkg_path"
	done
	printf '\n'
}

