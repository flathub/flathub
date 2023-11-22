# Carburetor flatpak recepie
This is the recepie for packaging [carburetor](https://tractor.frama.io/carburetor) as flatpak.

## Update dependencies
Fill all python requirements (except `PyGobject` as that one is already provided in `org.gnome.Platform` runtime) in `requirements.txt` file and use [flatpak pip generator](https://github.com/flatpak/flatpak-builder-tools/tree/master/pip) to update `python3-requirements.yaml` as below:

    python3 flatpak-pip-generator --requirements-file=requirements.txt --yaml

## Build the flatpak
    flatpak-builder --force-clean build-dir io.frama.tractor.carburetor.yaml

## Install the flatpak
    flatpak-builder --user --install --force-clean build-dir io.frama.tractor.carburetor.yaml

## Run the flatpak
    flatpak run io.frama.tractor.carburetor
