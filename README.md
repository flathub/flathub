# TntConnect Flatpak Package

This is an attempt to package TntConnect as flatpack package.

## Prerequisites

- Install flatpak and flatpak-builder
- Install the used runtime and sdk (currently `org.freedesktop.* 21.08`)

## Building

To build the flatpak, run the following command:

```bash
flatpak-builder --force-clean --ccache --repo=tntconnect-repo build-dir com.tntware.TntConnect.yml
```

To run the created flatpak:

```bash
flatpak-builder --run build-dir com.tntware.TntConnect.yml tntconnect.sh
```
