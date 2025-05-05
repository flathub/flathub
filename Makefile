build:
	flatpak-builder --user --force-clean build-dir io.github.jacalz.hegelmote.yml

install:
	flatpak-builder --user --install --force-clean build-dir io.github.jacalz.hegelmote.yml

run:
	flatpak run --user io.github.jacalz.hegelmote
