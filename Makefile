build:
	flatpak run --command=flatpak-builder org.flatpak.Builder --user --force-clean build-dir io.github.heathcliff26.turbo-clicker.yaml
	flatpak build-export export build-dir
	flatpak build-bundle export io.github.heathcliff26.turbo-clicker.flatpak io.github.heathcliff26.turbo-clicker

install: build
	flatpak install --user -y io.github.heathcliff26.turbo-clicker.flatpak

uninstall:
	flatpak uninstall --user -y io.github.heathcliff26.turbo-clicker

run: install
	flatpak run --user io.github.heathcliff26.turbo-clicker

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest io.github.heathcliff26.turbo-clicker.yaml

update: update-manifest cargo-sources

update-manifest:
	flatpak run org.flathub.flatpak-external-data-checker io.github.heathcliff26.turbo-clicker.yaml

cargo-sources:
	./update-cargo-sources.sh

clean:
	rm -rf .flatpak-builder build-dir export *.flatpak flatpak-builder-tools/ Cargo.lock
