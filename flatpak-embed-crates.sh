#!/bin/sh
#
# Embed a config.toml into a tarball which uses vendored crates from the
# flatpak-cargo-generator script.

# Create a temporary directory for the archive
TMPDIR=$(mktemp -d)
function cleanup(){
  rm -rf $TMPDIR
}
trap cleanup EXIT

# Decompress the tarball
case $1 in
  *.tar.gz)
    gunzip -c $1 > $TMPDIR/archive.tar
    ;;
  *)
    echo "Unsupported archive format: $1" >&2
    exit 1
    ;;
esac

# Embed the config file into the tarball
ROOTDIR=$(tar tf $TMPDIR/archive.tar | head -1 | cut -f1 -d/)
mkdir -p $TMPDIR/$ROOTDIR/.cargo
cat << EOF > $TMPDIR/$ROOTDIR/.cargo/config.toml
[source.vendored-sources]
directory = "${PWD}/cargo/vendor"

[source.crates-io]
replace-with = "vendored-sources"
EOF
tar -rf $TMPDIR/archive.tar -C $TMPDIR $ROOTDIR

# Recompress the tarball
case $1 in
  *.tar.gz)
    gzip -c $TMPDIR/archive.tar > $1
    ;;
  *)
    echo "Unsupported archive format: $1" >&2
    exit 1
    ;;
esac
