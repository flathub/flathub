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
	wget -N -O "Grayjay.Desktop-linux-x64.zip" https://updater.grayjay.app/Apps/Grayjay.Desktop/Grayjay.Desktop-linux-x64.zip

	mkdir -p ~/.var/app/app.grayjay.Desktop/data/Grayjay/
	unzip -o "Grayjay.Desktop-linux-x64.zip" "Grayjay.Desktop-linux-x64-v5/grayjay.png" -d "./"
	unzip -o "Grayjay.Desktop-linux-x64.zip" "Grayjay.Desktop-linux-x64-v5/wwwroot/*" -d "./"

	cp -r Grayjay.Desktop-linux-x64-v5/* ~/.var/app/app.grayjay.Desktop/data/Grayjay/


	rm -f "Grayjay.Desktop-linux-x64.zip"

unpatch:
	rm -r ~/.var/app/app.grayjay.Desktop/data/Grayjay/wwwroot/
	rm ~/.var/app/app.grayjay.Desktop/data/Grayjay/grayjay.png