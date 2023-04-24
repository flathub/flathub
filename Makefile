.PHONY: all generate-dependencies build bundle install

all: generate-dependencies build bundle install

generate-dependencies:
	python flatpak-pip-generator-fix --runtime='org.freedesktop.Sdk//22.08' --yaml --output pypi-dependencies --requirements-file='requirements.txt'

build:
	flatpak-builder --repo=myrepo --force-clean build-dir io.github.voxelcubes.deepqt.yaml

bundle:
	flatpak build-bundle myrepo deepqt.flatpak io.github.voxelcubes.deepqt

install:
	flatpak install --user deepqt.flatpak

run:
	flatpak run io.github.voxelcubes.deepqt

clean:
	rm -rf build-dir myrepo deepqt.flatpak .flatpak-builder
