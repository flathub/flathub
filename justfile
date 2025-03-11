build:
	flatpak-builder --user --install --force-clean build-dir app.grayjay.Grayjay.yaml


build-sandbox:
	flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo ./build-dir app.grayjay.Grayjay.yaml


run:
	flatpak run app.grayjay.Grayjay

debugshell:
	flatpak-builder --run ./build-dir ./app.grayjay.Grayjay.yaml sh

bundle:
	flatpak build-bundle ~/.local/share/flatpak/repo GrayjayDesktop.flatpak app.grayjay.Grayjay

prep-npm:
	flatpak-builder --run build-dir ./app.grayjay.Grayjay.yaml ./scripts/npm-deps.sh npm-sources.json /run/build/grayjay/Grayjay.Desktop.Web/package-lock.json

# this expects to be run in a full clone of the grayjay desktop repo tree checked out on the host machine
# also, do not set the runtimes here. It creates missing dependencies that need to be looked into (something weird with macos and windows dependencies probably being mislabeled for linux or something)
prep-nuget:
	python3 ./flatpak-builder-tools/dotnet/flatpak-dotnet-generator.py nuget-sources.json ../Grayjay.Desktop/Grayjay.Desktop.CEF/Grayjay.Desktop.CEF.csproj --freedesktop 24.08 --dotnet 8

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest ./app.grayjay.Grayjay.yaml
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
