## Flatpak specific notes

Gamepads will not work in the Flatpak version (we believe this has the same cause as https://github.com/flathub/org.chromium.Chromium/issues/40)

However, there is a workaround by giving read-only access to `/run/udev` with Flatseal or by running this command:

```bash
flatpak override org._02engine._02Engine --filesystem=/run/udev:ro
```

By default, drag and drop will only work with files in your downloads, pictures, music, or desktop folders. To allow other folders, run this command:

```bash
flatpak override org._02engine._02Engine --filesystem=/path/to/folder/
```
