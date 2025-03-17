deps:
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo\n
	flatpak install flathub org.gnome.Platform//46 org.gnome.Sdk//46

build:
	flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/media/ --repo=repo builddir sh.loft.devpod.yaml

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo

bundle:
	flatpak build-bundle ./repo devpod.flatpak sh.loft.devpod

install:
	flatpak install --user devpod.flatpak

run:
	flatpak run --user sh.loft.devpod

clean:
	rm -rf repo builddir devpod.flatpak .flatpak-builder
