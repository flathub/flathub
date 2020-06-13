# Proton flatpak

Proton is the Valve's compatibility layer for running Windows games on Linux through Steam.

This repo contains recepies for building Proton from source on top of [freedesktop-sdk](https://gitlab.com/freedesktop-sdk/freedesktop-sdk) in flatpak format, intended for running from the [Steam flatpak](https://github.com/flathub/com.valvesoftware.Steam).

## Installation
Flatpak 1.7.3+ is required.

This build is unofficial and wasn't tested with all possible games and cases. It can behave differently from official builds and possibly cause data loss (e.g. break saved games). Use at your own risk.

First [add](https://flatpak.org/setup) Flathub repository and install Steam from there, if not already. Then run
```
flatpak remote-add --user proton-flatpak https://gasinvein.github.io/proton-flatpak
flatpak install proton-flatpak com.valvesoftware.Steam.CompatibilityTool.Proton
```
(remove `--user` option if you're installing it system-wide)

## Running

Launch Steam flatpak and select "Proton (flatpak)" compatibility tool from drop-down list.
