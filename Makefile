all: generated-sources flatpak

.PHONY: generated-sources
generated-sources:
	./update-sources.sh

.PHONY: build-flatpak
flatpak:
	flatpak-builder --force-clean build --user --install ./io.kinvolk.Headlamp.yaml
