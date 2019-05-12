# set distribution-specific package architecture for Arch Linux target
# USAGE: set_architecture_arch $architecture
# CALLED BY: set_architecture
set_architecture_arch() {
	case "$1" in
		('32'|'64')
			pkg_architecture='x86_64'
		;;
		(*)
			pkg_architecture='any'
		;;
	esac
}

