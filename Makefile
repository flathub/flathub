build:
	flatpak run org.flatpak.Builder --user --install --force-clean build-dir org.fcast.Receiver.yaml

build-offline:
	flatpak run org.flatpak.Builder --user --install --disable-download --force-clean build-dir org.fcast.Receiver.yaml

build-sandbox:
	flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo ./build-dir org.fcast.Receiver.yaml

run:
	flatpak run org.fcast.Receiver

debugshell:
	flatpak-builder --run ./build-dir ./org.fcast.Receiver.yaml sh

prep-npm:
	./scripts/npm-deps.sh https://gitlab.futo.org/videostreaming/fcast/-/raw/master/receivers/electron

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream build-dir/files/share/metainfo/org.fcast.Receiver.metainfo.xml
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest ./org.fcast.Receiver.yaml
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder builddir build-dir
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo
