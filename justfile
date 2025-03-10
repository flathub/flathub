build:
	flatpak-builder --user --install --force-clean build-dir app.grayjay.Desktop.yaml

run:
	flatpak run app.grayjay.Desktop

debugshell:
	flatpak-builder --run ./build-dir ./app.grayjay.Desktop.yaml sh

bundle:
	flatpak build-bundle ~/.local/share/flatpak/repo GrayjayDesktop.flatpak app.grayjay.Desktop

prep-npm:
	flatpak-builder --run build-dir ./app.grayjay.Desktop.yaml ./scripts/npm-deps.sh npm-sources.json /run/build/grayjay/Grayjay.Desktop.Web/package-lock.json
