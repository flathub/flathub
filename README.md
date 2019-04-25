# de.ifw_dresden.nagstamon

This repo contains the flatpak version of Nagstamon (https://github.com/HenriWahl/Nagstamon)

## How to build Nagstamon

```bash
flatpak-builder --repo=local-repo --force-clean build-dir de.ifw_dresden.nagstamon.yml
```

## Add Nagstamon repo to remote

```bash
flatpak --user remote-add --no-gpg-verify local-repo local-repo
```

## How to install Nagstamon from flatpak

```bash
flatpak --user install local-repo de.ifw_dresden.nagstamon
```

## How to run Nagstamon
```bash
flatpak run de.ifw_dresden.nagstamon
```

## How to uninstall Nagstamon

```bash
flatpak uninstall --user de.ifw_dresden.nagstamon
```