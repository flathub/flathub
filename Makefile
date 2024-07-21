.PHONY: build test

build:
	echo "starting flatpak build..."
	flatpak-builder \
		--repo=repo \
		--user \
		--disable-rofiles-fuse \
		--install-deps-from=flathub \
		--force-clean \
		--default-branch=master \
		--arch=x86_64 \
		--install builddir/ \
		dev.overlayed.Overlayed.yaml
	
test:
	flatpak run dev.overlayed.Overlayed