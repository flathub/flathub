set -x
set -e
registry=npmjs.org/
other=github.com/

# For NPM packages, in addition to the CAFS entries for each file,
# an index is stored in the CAFS but with `-index.json` appended:
# 	store/v3/files/ab/cdef...-index.json
# This index is referenced by another per-package file outside the CAFS at
# 	store/v3/file+path+to+package-version.tgz/tarball-integrity
# which contains the base64 encoded hash of said index.  The command
# 	pnpm store add ./path/to/npm-registry/package.tgz
# does just that:  Extract all downloaded packages and add them into the CAFS
# under
# 	store/v3/files/ab/cdef...
#
#registry=$1
#
#for a in $(find "$registry" -type f); do
#	./pnpm store add "./$a"
#done

# However, if we add packages from outside the registry e.g., GitHub,
# Packages added through the "remote tarball" [^1] package location handler,
# the `pnpm i` command later expects the `...-index.json` be symlinked directly
# from outside the CAFS by a link called
#	store/v3/domain+of+package@commit/integrity.json
#
# [1]: https://pnpm.io/cli/add#install-from-remote-tarball

#XDG_DATA_HOME=${XDG_DATA_HOME:=$HOME/.local/share}
#store="$XDG_DATA_HOME/pnpm/store"
store="./.pnpm-store"
for a in $(find "$other" -type f); do

	# 0. Add it through "local filesystem" package source
	./pnpm store --store-dir "$store" add "./$a"

	# 1. Find the `tarball-integrity` file and decode it's hash,
	archivename=$(echo "$a" | tr '/' '+')
	name="${archivename%.tar.gz}"
	integrity="$store/v3/file+$archivename/tarball-integrity"
	hash=$(cut -d- -f2 < "$integrity" | base64 -d | od -tx1 -An | tr -d ' \n' )

	# 2. Look the hash up in the CAFS, and symlink it to the link `integrity.json`
	hdir=$(echo "$hash" | cut -c 1-2)
	hname=$(echo "$hash" | cut -c 3-)
	mkdir -p "$store/v3/$name"
	ln -sf "../files/$hdir/$hname-index.json" \
		"$store/v3/$name/integrity.json"
done
