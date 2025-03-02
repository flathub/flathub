build:
	flatpak-builder --user --install --force-clean build-dir app.grayjay.Desktop.yaml

run:
	flatpak run app.grayjay.Desktop

debugshell:
	flatpak-builder --run ./build-dir ./app.grayjay.Desktop.yaml sh