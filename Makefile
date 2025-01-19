deps:
	flatpak remote-add --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo\n
	flatpak install flathub org.gnome.Platform//46 org.gnome.Sdk//46

build:
	flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir org.loft.devpod.yaml

bundle:
	flatpak build-bundle ./repo devpod.flatpak org.loft.devpod

install:
	flatpak install --user devpod.flatpak

run:
	flatpak run --user org.loft.devpod

clean:
	rm -rf repo builddir devpod.flatpak
	