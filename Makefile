#!/bin/bash

build:
	flatpak-builder --user --repo=repo --install --force-clean build-dir org.learningequality.Kolibri.json

run:
	flatpak-builder --run build-dir org.learningequality.Kolibri.json run_kolibri.sh

start:
	flatpak run org.learningequality.Kolibri

stop:
	flatpak kill org.learningequality.Kolibri

clean:
	rm -r .flatpak-builder
	rm -r build-dir