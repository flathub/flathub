install:
	flatpak-builder --delete-build-dirs --force-clean --user --install build gg.guilded.Guilded.yml

uninstall:
	flatpak uninstall --delete-data gg.guilded.Guilded

clean:
	rm --recursive --force build .flatpak-builder
