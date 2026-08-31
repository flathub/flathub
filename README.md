# Flathub Submission: com.lupyd.client

This directory contains the official Flathub repository files for `com.lupyd.client`.

## Flathub PR Instructions

1. Fork https://github.com/flathub/flathub (for new app submissions via `new-pr` branch) or create repository `https://github.com/flathub/com.lupyd.client`.
2. Copy all files in this directory to your Flathub branch:
   - `com.lupyd.client.yml`
   - `com.lupyd.client.metainfo.xml`
   - `com.lupyd.client.desktop`
   - `com.lupyd.client-launcher.sh`
   - `flathub.json`
3. If publishing a new release, update `url` and `sha256` in `com.lupyd.client.yml` pointing to:
   `https://github.com/lupyd/lupyd/releases/download/v0.0.43/Lupyd_linux_x86-64.AppImage`
4. Submit the Pull Request to Flathub!

## Local Flatpak Build Test

To test building the Flatpak locally using `flatpak-builder`:

```bash
flatpak-builder --force-clean --user --install build-dir com.lupyd.client.yml
flatpak run com.lupyd.client
```
