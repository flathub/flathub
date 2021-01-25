local-install:
	flatpak-builder --user --install build-dir org.nitrokey.app.yml --force-clean

build:
	flatpak-builder build-dir org.nitrokey.app.yml --force-clean

run:
	flatpak run org.nitrokey.app

