local-install:
	flatpak-builder --user --install build-dir com.nitrokey.nitrokey-app.yml --force-clean

build:
	flatpak-builder build-dir com.nitrokey.nitrokey-app.yml --force-clean

run:
	flatpak run com.nitrokey.nitrokey-app

