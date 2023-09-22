# DSView Flatpak

Flatpak build of [DSView](https://github.com/DreamSourceLab/DSView).

## About udev

It is required to manually setup udev rule for USB devices.

Place the following contents into `/etc/udev/rules.d/60-dreamsourcelab.rules`:

```
SUBSYSTEM=="usb", ATTRS{idVendor}=="2a0e", MODE="0666"
```

## Locally build & install

Assuming `flatpak-builder` is installed, then simply run:

```sh
flatpak-builder --user --install --force-clean build com.dreamsourcelab.DSView.yml
```

This will create a build directory named `build`.

## TODOs

- Minimize boost
- Upstream CMakeLists.txt patch
