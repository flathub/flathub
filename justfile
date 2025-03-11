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

# TODO: this breaks if things are done fully from scratch with no prior builds
prep-nuget:
	DOTNET_CLI_TELEMETRY_OPTOUT=true DOTNET_SKIP_FIRST_TIME_EXPERIENCE=true flatpak-builder --run build-dir ./app.grayjay.Grayjay.yaml dotnet restore --packages ./packages /run/build/grayjay/Grayjay.Desktop.CEF/Grayjay.Desktop.CEF.csproj
	flatpak-builder --run build-dir ./app.grayjay.Grayjay.yaml python3 ./scripts/flatpak-dotnet-generator.py nuget-sources.json /run/build/grayjay/Grayjay.Desktop.CEF/Grayjay.Desktop.CEF.csproj
	rm -rf ./packages

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest ./app.grayjay.Grayjay.yaml
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
