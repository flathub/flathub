sdk-dep:
	flatpak install flathub org.kde.Sdk//5.15

build: sdk-dep
	flatpak-builder --user --install --force-clean build-dir io.parmentier.flatpak.streamdeck.yml

run:
	flatpak run io.parmentier.flatpak.streamdeck
