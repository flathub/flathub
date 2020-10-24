# Proton flatpak

Proton is the Valve's compatibility layer for running Windows games on Linux through Steam.

This repository contains recepies for building Proton from source on top of [freedesktop-sdk](https://gitlab.com/freedesktop-sdk/freedesktop-sdk) in flatpak format, intended for running from the [Steam flatpak](https://github.com/flathub/com.valvesoftware.Steam).

## Installation

This unofficial build isn't supported by Valve and wasn't tested with all possible games and cases. It can behave differently from official builds. Use at your own risk.

First [add](https://flatpak.org/setup) Flathub repository and install Steam from there, if not already. Then run
```
flatpak install com.valvesoftware.Steam.CompatibilityTool.Proton
```

## Running

Launch Steam flatpak and select "Proton (flatpak)" compatibility tool from drop-down list.
