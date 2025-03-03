build:
	flatpak-builder --user --install build-dir app.grayjay.Desktop.yaml

clean-build:
	flatpak-builder --user --install --force-clean build-dir app.grayjay.Desktop.yaml

run:
	flatpak run app.grayjay.Desktop

debugshell:
	flatpak-builder --run ./build-dir ./app.grayjay.Desktop.yaml sh


bundle:
	flatpak build-bundle ~/.local/share/flatpak/repo GrayjayDesktop.flatpak app.grayjay.Desktop

patch:
	./patch.sh

unpatch:
	./unpatch.sh