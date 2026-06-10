# UniRTM Flatpak

This repository contains the Flatpak manifest for [UniRTM](https://github.com/snowdreamtech/UniRTM).

## About UniRTM

UniRTM is a universal runtime manager that helps you manage multiple toolchains, SDKs, and runtimes with a unified interface.

## Installation

To install UniRTM as a Flatpak:

```bash
flatpak install flathub io.github.snowdreamtech.UniRTM
```

## Building from Source

To build the Flatpak locally:

```bash
flatpak-builder --user --install build-dir io.github.snowdreamtech.UniRTM.yml
```

## License

UniRTM is licensed under the MIT License.
