.PHONY: generate-dependencies build-install run lint clean

# Building requires:
# $ flatpak install -y flathub org.flatpak.Builder
# $ flatpak remote-add --user --if-not-exists flathub https://flathub.org/repo/flathub.flatpakrepo

ID := io.github.voxelcubes.hand-tex


generate-dependencies:
	python flatpak-pip-generator-fix --runtime='org.kde.Sdk//6.7' --yaml --output pypi-dependencies --requirements-file='requirements.txt'

build-install:
	flatpak run org.flatpak.Builder --force-clean --sandbox --user --install --install-deps-from=flathub --ccache --mirror-screenshots-url=https://dl.flathub.org/repo/screenshots --repo=repo builddir $(ID).yaml

run:
	flatpak run $(ID)

lint:
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder manifest $(ID).yaml
	flatpak run --command=flatpak-builder-lint org.flatpak.Builder --exceptions repo repo

clean:
	rm -rf builddir repo panelcleaner.flatpak .flatpak-builder

introspect:
	flatpak run --command=sh --devel $(ID)
