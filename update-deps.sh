#!/bin/sh

if test ! -d patchpal-gui/.git ; then
	git clone https://gitlab.com/patchpal-ai/patchpal-gui.git || exit 1
fi

pushd patchpal-gui
rm -f Cargo.lock && cargo generate-lockfile
cargo update
~/Projects/jhbuild/flatpak-builder-tools/cargo/flatpak-cargo-generator.py Cargo.lock
mv generated-sources.json ..
popd
