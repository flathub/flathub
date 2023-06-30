This is the repo for the Flatpak version of [Bforartists](https://www.bforartists.de/), based on https://github.com/flathub/org.blender.Blender

# Building Instructions
1. Install `flatpak` and `flatpak-builder` from your distro's repo
2. Install the SDK `flatpak install flathub org.freedesktop.Platform//22.08 org.freedesktop.Sdk//22.08`
3. Clone the repo (with `--recurse-submodules` for the shared Flatpak modules)
4. Create a new folder called `build-dir`
5. To build for testing, execute `flatpak-builder --user --install --force-clean build-dir de.bforartists.Bforartists.json`. This will build and install Bforartists.

Note that FFMPEG will be downloaded and compiled too.

# Upgrading to the Newest Version
Open `org.bforartists.Bforartists.json` and scroll down to this section:
```json
    {
        "type": "archive",
        "url": "URL FOR BFA ARCHIVE ON GITHUB RELEASES",
        "sha256": "SHA256 CHECKSUM FOR BFA ARCHIVE",
        "strip-components": 0 
    },
``` 

Replace the `url` variable with the url of the latest archive (much be from GitHub releases). Replace `sha256` with the result of `sha256sum <Bforartists tar archive>`. In addition, it's important to check the upstream Blender flatpak repo to see if anything in the manifest has changed.
