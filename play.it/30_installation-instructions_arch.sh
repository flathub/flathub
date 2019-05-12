# print installation instructions for Arch Linux
# USAGE: print_instructions_arch $pkg[…]
print_instructions_arch() {
	local pkg_path
	local str_format
	printf 'pacman -U'
	for pkg in "$@"; do
		if [ "$OPTION_ARCHITECTURE" != all ] && [ -n "${PACKAGES_LIST##*$pkg*}" ]; then
			skipping_pkg_warning 'print_instructions_arch' "$pkg"
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

