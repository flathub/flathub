# Streamdeck 

Flatpak application for [streamdeck-ui](https://github.com/timothycrosley/streamdeck-ui)

In order to work, Update udev rules `/etc/udev/rules.d/70-streamdeck.rules`:
```
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0060", TAG+="uaccess"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0063", TAG+="uaccess"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="006c", TAG+="uaccess"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="006d", TAG+="uaccess"
SUBSYSTEM=="usb", ATTRS{idVendor}=="0fd9", ATTRS{idProduct}=="0080", TAG+="uaccess"
```

Then reload the rules `sudo udevadm control --reload-rules`

## Build the package

```
make build
```

## Useful links

It helps me to make this flatpak app working.

  - https://github.com/flatpak/flatpak-builder-tools/issues/239#issuecomment-971430197
  - [Flatpak - Steam](https://github.com/flathub/com.valvesoftware.Steam)
  - https://github.com/flathub/org.freecadweb.FreeCAD/blob/master/org.freecadweb.FreeCAD.yaml
  - https://code.qt.io/cgit/pyside/pyside-setup.git/
  - https://invent.kde.org/packaging/flatpak-kde-runtime/-/merge_requests/32/diffs / https://bugs.kde.org/show_bug.cgi?id=391748
  - https://www.reddit.com/r/Fedora/comments/ams0xd/making_a_flatpak_for_flashprint_issue_with/
  - https://github.com/flathub/flathub/pull/840/files
  - https://github.com/flathub/com.leinardi.gkraken/blob/fc8c300d3d4eea32b25c42ab0f11c45ef286f238/com.leinardi.gkraken.json

