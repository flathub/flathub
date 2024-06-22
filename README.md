# Flathub

Flathub is the central place for building and hosting Flatpak builds.

# SuperCollider Flatpak

* [SuperCollider](https://supercollider.github.io/)
* [Flatpak](https://www.flatpak.org/)

## Build SuperCollider package

Install dependencies:

```sh
flatpak install org.kde.Sdk//5.15-22.08
```

Build the package
```sh
 flatpak-builder --ccache --repo=flatpak_repo --force-clean build-dir io.github.supercollider.SuperCollider.yml
```

Build the bundle
```sh
 flatpak build-bundle flatpak_repo SuperCollider.flatpak io.github.SuperCollider
```

## Install bundle locally

```sh
flatpak install --user SuperCollider.flatpak
```

## Run

scide (default):

```sh
flatpak run io.github.SuperCollider
```

Run only `sclang`:
```sh
flatpak run --command=sclang io.github.SuperCollider path/to/file.sc
```


Using the Flathub repository
----------------------------

To install applications that are hosted on Flathub, use the following:
```
flatpak remote-add flathub https://flathub.org/repo/flathub.flatpakrepo
flatpak install flathub org.gnome.Recipes
```

To install applications from the beta branch, use the following:
```
flatpak remote-add flathub-beta https://flathub.org/beta-repo/flathub-beta.flatpakrepo
flatpak install flathub-beta org.godotengine.Godot
```

For more information and more applications see https://flathub.org

Contributing to Flathub
-----------------------

For information on creating packages or reporting issues please see the [contributing page](/CONTRIBUTING.md).

***Note:*** *this repository is not for reporting issues related to the flathub.org website itself or contributing to its development. For that, go to https://github.com/flathub-infra/website*
