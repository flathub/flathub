build:
	flatpak install -y org.kde.Sdk//5.15-23.08 org.kde.Platform//5.15-23.08 	
	flatpak-builder --ccache --force-clean --user --install build-dir io.github.ceprogramming.CEmu.yml
run:
	flatpak run io.github.ceprogramming.CEmu
clean:
	rm -rf .flatpak-builder
	rm -rf build-dir
uninstall:
	flatpak remove io.github.ceprogramming.CEmu --delete-data

.PHONY: build run clean uninstall
