#!/usr/bin/make -f

build:
	mkdir -p _source
	flatpak-builder \
		--force-clean \
		--repo=_build/repo \
		--extra-sources=_source \
		_build \
		com.valvesoftware.steamlink.yml
	flatpak \
		build-bundle \
		_build/repo \
		_build/steamlink.flatpak \
		com.valvesoftware.steamlink
	@echo "Install with: flatpak install --bundle /path/to/steamlink.flatpak"

clean:
	rm -fr _build
