# Proton flatpak

Proton is the Valve's compatibility layer for running Windows games on Linux through Steam.

This repository contains recepies for building Proton from source on top of [freedesktop-sdk](https://gitlab.com/freedesktop-sdk/freedesktop-sdk) in flatpak format, intended for running from the [Steam flatpak](https://github.com/flathub/com.valvesoftware.Steam).

## Installation

This unofficial build isn't supported by Valve and wasn't tested with all possible games and cases. It can behave differently from official builds. Use at your own risk.

Flatpak 1.7.3+ is required to install from an OCI repository. If you have older flatpak or don't want to add the repository - you can download flatpak bundle artifact from github [actions](https://github.com/gasinvein/proton-flatpak/actions?query=workflow%3AFlatpak).

First [add](https://flatpak.org/setup) Flathub repository and install Steam from there, if not already. Then run
```
flatpak remote-add --user proton-flatpak oci+https://gasinvein.github.io/proton-flatpak
flatpak install proton-flatpak com.valvesoftware.Steam.CompatibilityTool.Proton
```
(remove `--user` option if you're installing it system-wide)

## Running

Launch Steam flatpak and select "Proton (flatpak)" compatibility tool from drop-down list.
