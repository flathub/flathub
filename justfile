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

# TODO: this breaks if things are done fully from scratch with no prior builds
prep-nuget:
	flatpak-builder --run build-dir ./app.grayjay.Desktop.yaml dotnet restore --packages ./packages /run/build/grayjay/Grayjay.Desktop.CEF/Grayjay.Desktop.CEF.csproj
	flatpak-builder --run build-dir ./app.grayjay.Desktop.yaml python3 ./scripts/flatpak-dotnet-generator.py nuget-sources.json /run/build/grayjay/Grayjay.Desktop.CEF/Grayjay.Desktop.CEF.csproj
	rm -rf ./packages
