# Utility to test/validate this flatpak

FLATPAK_ID=de.helden_software.helden5

all: install

deps:
	flatpak remote-add --if-not-exists --user flathub https://dl.flathub.org/repo/flathub.flatpakrepo
	flatpak install -y --user flathub org.flatpak.Builder

install:
	flatpak-builder --force-clean --user --install-deps-from=flathub --repo=repo --install builddir ${FLATPAK_ID}.yml

uninstall:
	flatpak uninstall --user -y ${FLATPAK_ID}

bundle:
	flatpak build-bundle repo helden-software.flatpak ${FLATPAK_ID} --runtime-repo=https://flathub.org/repo/flathub.flatpakrepo

run: install
	flatpak run ${FLATPAK_ID}

validate:
	desktop-file-validate *.desktop
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder appstream ${FLATPAK_ID}.metainfo.xml
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest ${FLATPAK_ID}.yml
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder repo repo

clean:
	@rm -rf dist builddir .flatpak-builder
